#include "oath_storage.h"
#include "../crypto/aes.h"
#include "../crypto/aes_gcm.h"
#include "../crypto/sha256.h"
#include "security/security.h"
#include <hardware/address_mapped.h>
#include <hardware/flash.h>
#include <hardware/sync.h>
#include <pico/rand.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>

/**
 * @file oath_storage.c
 * @brief Secure encrypted flash storage for OATH credentials.
 *
 * Implements AES-GCM encryption per credential and full-block encryption
 * for the storage index.
 */

#include "security/security_manager.h"

#define STORAGE_MAGIC 0x534F4154 // "SOAT" (Secure OATH)
#define STORAGE_VERSION 0x02

// Calculate padded size for encryption (must be multiple of AES_BLOCK_SIZE)
#define PADDED_PERSIST_SIZE                                                    \
  ((sizeof(oath_persist_t) + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE) *           \
      AES_BLOCK_SIZE

typedef struct {
  uint8_t iv[AES_IV_SIZE_BYTES];
  uint8_t tag[16];
  uint8_t encrypted_data[PADDED_PERSIST_SIZE];
} oath_flash_package_t;

static oath_persist_t ram_cache;
static oath_flash_package_t flash_buffer;

// Forward declarations
static bool save_to_flash(void);
static bool load_from_flash(void);
static bool derive_and_validate_key(uint8_t *key_out);

//--------------------------------------------------------------------+
// Encryption Helpers
//--------------------------------------------------------------------+

static bool derive_and_validate_key(uint8_t *key_out) {
  if (!otp_read_master_key(key_out)) {
    printf("[STORAGE] Master key missing. Attempting to provision...\n");
    if (!otp_write_new_master_key(key_out)) {
      printf("[STORAGE] CRITICAL: Hard failure provisioning master key.\n");
      return false;
    }
  }
  return true;
}

bool encrypt_credential(const uint8_t *key, const oath_credential_t *cred,
                        encrypted_credential_t *encrypted) {
  // 1. Generate unique IV per credential
  for (int i = 0; i < 12; i++) {
    encrypted->iv[i] = (uint8_t)(get_rand_32() & 0xFF);
  }

  // 2. AES-GCM Authenticated Encryption
  return aes_gcm_encrypt(key, encrypted->iv, (const uint8_t *)cred,
                         sizeof(oath_credential_t), encrypted->ciphertext,
                         encrypted->tag);
}

bool decrypt_credential(const uint8_t *key,
                        const encrypted_credential_t *encrypted,
                        oath_credential_t *cred) {
  return aes_gcm_decrypt(key, encrypted->iv, encrypted->ciphertext,
                         sizeof(oath_credential_t), encrypted->tag,
                         (uint8_t *)cred);
}

//--------------------------------------------------------------------+
// File System Logic
//--------------------------------------------------------------------+

static bool save_to_flash(void) {
  uint8_t master_key[32];
  if (!derive_and_validate_key(master_key))
    return false;

  printf("[STORAGE] Syncing encrypted index to flash...\n");

  // 1. Prepare flash package
  for (int i = 0; i < AES_IV_SIZE_BYTES / 4; i++) {
    ((uint32_t *)flash_buffer.iv)[i] = get_rand_32();
  }

  // 2. Encrypt the entire index using AES-GCM (Authenticated Encryption)
  // This ensures both confidentiality and integrity of the credential list.
  uint8_t temp_aligned[PADDED_PERSIST_SIZE];
  memset(temp_aligned, 0, PADDED_PERSIST_SIZE);
  memcpy(temp_aligned, &ram_cache, sizeof(oath_persist_t));

  bool success = aes_gcm_encrypt(master_key, flash_buffer.iv, temp_aligned,
                                 PADDED_PERSIST_SIZE,
                                 flash_buffer.encrypted_data, flash_buffer.tag);

  if (!success) {
    printf("[STORAGE] Encryption failed during flash sync.\n");
    return false;
  }

  // 3. Hardware Write
  uint32_t ints = save_and_disable_interrupts();
  flash_range_erase(OATH_FLASH_SECTOR_OFFSET, FLASH_SECTOR_SIZE);
  flash_range_program(OATH_FLASH_SECTOR_OFFSET, (const uint8_t *)&flash_buffer,
                      sizeof(oath_flash_package_t));
  restore_interrupts(ints);

  return true;
}

static bool load_from_flash(void) {
  const oath_flash_package_t *stored =
      (const oath_flash_package_t *)(XIP_BASE + OATH_FLASH_SECTOR_OFFSET);

  uint8_t master_key[32];
  if (!derive_and_validate_key(master_key))
    return false;

  uint8_t temp_aligned[PADDED_PERSIST_SIZE];
  bool decrypted =
      aes_gcm_decrypt(master_key, stored->iv, stored->encrypted_data,
                      PADDED_PERSIST_SIZE, stored->tag, temp_aligned);

  if (decrypted) {
    oath_persist_t *temp_persist = (oath_persist_t *)temp_aligned;
    if (temp_persist->magic == STORAGE_MAGIC) {
      memcpy(&ram_cache, temp_persist, sizeof(oath_persist_t));
      printf("[STORAGE] Secure storage loaded. (%d credentials found)\n",
             ram_cache
                 .version); // Just using version field for logging temporarily
      return true;
    }
  }

  printf("[STORAGE] No valid encrypted storage found. Initializing factory "
         "state.\n");
  memset(&ram_cache, 0, sizeof(oath_persist_t));
  ram_cache.magic = STORAGE_MAGIC;
  ram_cache.version = STORAGE_VERSION;
  return save_to_flash();
}

void oath_storage_init(void) { load_from_flash(); }

bool oath_storage_put(const char *name, const uint8_t *secret,
                      uint8_t secret_len, oath_type_t type, oath_algo_t algo,
                      uint8_t digits, uint32_t period, uint8_t touch_required) {

  if (secret_len > OATH_MAX_SECRET_LEN)
    return false;

  oath_credential_t new_cred;
  memset(&new_cred, 0, sizeof(new_cred));
  strncpy(new_cred.name, name, OATH_MAX_NAME_LEN - 1);
  memcpy(new_cred.secret, secret, secret_len);
  new_cred.secret_len = secret_len;
  new_cred.type = type;
  new_cred.algorithm = algo;
  new_cred.digits = digits;
  new_cred.period = period;
  new_cred.touch_required = touch_required;

  uint8_t key[32];
  if (!derive_and_validate_key(key))
    return false;

  encrypted_credential_t encrypted;
  if (!encrypt_credential(key, &new_cred, &encrypted))
    return false;

  // Slot Management logic
  int i;
  int free_slot = -1;

  for (i = 0; i < MAX_CREDENTIALS; i++) {
    if (ram_cache.slot_used[i]) {
      oath_credential_t existing;
      if (decrypt_credential(key, &ram_cache.encrypted_creds[i], &existing)) {
        if (strcmp(existing.name, name) == 0) {
          memcpy(&ram_cache.encrypted_creds[i], &encrypted, sizeof(encrypted));
          return save_to_flash();
        }
      }
    } else if (free_slot == -1) {
      free_slot = i;
    }
  }

  if (free_slot != -1) {
    ram_cache.slot_used[free_slot] = true;
    memcpy(&ram_cache.encrypted_creds[free_slot], &encrypted,
           sizeof(encrypted));
    return save_to_flash();
  }

  return false;
}

bool oath_storage_delete(const char *name) {
  uint8_t key[32];
  if (!derive_and_validate_key(key))
    return false;

  for (int i = 0; i < MAX_CREDENTIALS; i++) {
    if (ram_cache.slot_used[i]) {
      oath_credential_t cred;
      if (decrypt_credential(key, &ram_cache.encrypted_creds[i], &cred)) {
        if (strcmp(cred.name, name) == 0) {
          ram_cache.slot_used[i] = false;
          memset(&ram_cache.encrypted_creds[i], 0,
                 sizeof(encrypted_credential_t));
          return save_to_flash();
        }
      }
    }
  }
  return false;
}

bool oath_storage_get(const char *name, oath_credential_t *out_cred) {
  uint8_t key[32];
  if (!derive_and_validate_key(key))
    return false;

  for (int i = 0; i < MAX_CREDENTIALS; i++) {
    if (ram_cache.slot_used[i]) {
      oath_credential_t cred;
      if (decrypt_credential(key, &ram_cache.encrypted_creds[i], &cred)) {
        if (strcmp(cred.name, name) == 0) {
          if (out_cred)
            memcpy(out_cred, &cred, sizeof(oath_credential_t));
          return true;
        }
      }
    }
  }
  return false;
}

const char *oath_storage_list(uint32_t index) {
  static char cached_name[OATH_MAX_NAME_LEN];
  uint32_t current_idx = 0;
  uint8_t key[32];
  if (!derive_and_validate_key(key))
    return NULL;

  for (int i = 0; i < MAX_CREDENTIALS; i++) {
    if (ram_cache.slot_used[i]) {
      if (current_idx == index) {
        oath_credential_t cred;
        if (decrypt_credential(key, &ram_cache.encrypted_creds[i], &cred)) {
          strncpy(cached_name, cred.name, OATH_MAX_NAME_LEN - 1);
          return cached_name;
        }
      }
      current_idx++;
    }
  }
  return NULL;
}

void oath_storage_reset(void) {
  memset(&ram_cache, 0, sizeof(oath_persist_t));
  ram_cache.magic = STORAGE_MAGIC;
  ram_cache.version = STORAGE_VERSION;
  save_to_flash();
}

bool oath_storage_update_counter(const char *name, uint32_t new_counter) {
  uint8_t key[32];
  if (!derive_and_validate_key(key))
    return false;

  for (int i = 0; i < MAX_CREDENTIALS; i++) {
    if (ram_cache.slot_used[i]) {
      oath_credential_t cred;
      if (decrypt_credential(key, &ram_cache.encrypted_creds[i], &cred)) {
        if (strcmp(cred.name, name) == 0) {
          cred.counter = new_counter;
          encrypted_credential_t encrypted;
          if (encrypt_credential(key, &cred, &encrypted)) {
            memcpy(&ram_cache.encrypted_creds[i], &encrypted,
                   sizeof(encrypted));
            return save_to_flash();
          }
        }
      }
    }
  }
  return false;
}

bool oath_storage_set_password(const uint8_t *code, uint8_t len) {
  if (len > 64)
    return false;
  SHA256_CTX ctx;
  SHA256Init(&ctx);
  SHA256Update(&ctx, code, len);
  SHA256Final(&ctx, ram_cache.access_code_hash);
  ram_cache.access_code_set = 1;
  return save_to_flash();
}

bool oath_storage_verify_password(const uint8_t *code, uint8_t len) {
  if (ram_cache.access_code_set == 0)
    return true;
  uint8_t hash[32];
  SHA256_CTX ctx;
  SHA256Init(&ctx);
  SHA256Update(&ctx, code, len);
  SHA256Final(&ctx, hash);

  volatile uint8_t diff = 0;
  for (int i = 0; i < 32; i++) {
    diff |= hash[i] ^ ram_cache.access_code_hash[i];
  }
  return (diff == 0);
}

bool oath_storage_is_password_set(void) {
  return (ram_cache.access_code_set == 1);
}
