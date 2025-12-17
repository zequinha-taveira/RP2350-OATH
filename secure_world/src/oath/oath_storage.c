#include "oath_storage.h"
#include "crypto/aes.h"
#include "crypto/aes_gcm.h"
#include "crypto/sha256.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/rand.h"
#include "pico/stdlib.h"
#include "security/security.h"
#include <stdio.h>
#include <string.h>

// Flash Storage Configuration
// Using the last 4KB sector of Flash for OATH data
#define FLASH_TARGET_OFFSET (2 * 1024 * 1024 - 4096)

// Calculate padded size for encryption
#define PADDED_DATA_SIZE                                                       \
  ((sizeof(oath_persist_t) + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE) *           \
      AES_BLOCK_SIZE

// Structure to be written to flash (IV + Encrypted Data)
typedef struct {
  uint8_t iv[AES_IV_SIZE_BYTES];
  uint8_t encrypted_data[PADDED_DATA_SIZE];
} oath_flash_data_t;

static oath_persist_t ram_cache;
static oath_flash_data_t flash_data_buffer; // Buffer for encryption/decryption

// Forward declarations
static void save_to_flash(void);

// Helper to derive master key from OTP
static bool derive_master_key(uint8_t *key_out) {
  if (!otp_read_master_key(key_out)) {
    // Generate new key if not exists
    if (!otp_write_new_master_key(key_out)) {
      printf("OATH Storage: Failed to generate master key\n");
      return false;
    }
  }
  return true;
}

// C-style encryption helpers using AES-GCM
bool encrypt_credential(const uint8_t *key, const oath_credential_t *cred,
                        encrypted_credential_t *encrypted) {
  // Generate IV
  for (int i = 0; i < 12; i++) {
    encrypted->iv[i] = (uint8_t)(get_rand_32() & 0xFF);
  }

  // Prepare plaintext
  uint8_t plaintext[256]; // Max size
  // For now we serialize the whole struct, or just parts?
  // Let's encrypt the sensitivity parts: secret, counters, etc.
  // Simpler to just encrypt the whole struct bytes for storage, excluding name
  // if we want lookup? oath_storage_put logic suggests name is separate or
  // decrypted to check. The current oath_credential_t has name inside.

  // Serializing:
  // We'll copy the credential struct to a flat buffer.
  memcpy(plaintext, cred, sizeof(oath_credential_t));

  return aes_gcm_encrypt(key, encrypted->iv, plaintext,
                         sizeof(oath_credential_t), encrypted->ciphertext,
                         encrypted->tag);
}

bool decrypt_credential(const uint8_t *key,
                        const encrypted_credential_t *encrypted,
                        oath_credential_t *cred) {
  uint8_t plaintext[256];

  if (!aes_gcm_decrypt(
          key, encrypted->iv, encrypted->ciphertext,
          sizeof(
              oath_credential_t), // We assume fixed size for now or store len
          encrypted->tag, plaintext)) {
    return false;
  }

  memcpy(cred, plaintext, sizeof(oath_credential_t));
  return true;
}

// Helper to write cache to flash
static void save_to_flash(void) {
  uint8_t master_key[AES_KEY_SIZE_BYTES];
  if (!derive_master_key(master_key)) {
    printf("OATH Storage: ERROR - Could not read Master Key for encryption!\n");
    return;
  }

  // 1. Generate a new random IV
  for (int i = 0; i < AES_IV_SIZE_BYTES / 4; i++) {
    ((uint32_t *)flash_data_buffer.iv)[i] = get_rand_32();
  }

  // 2. Encrypt ram_cache into flash_data_buffer.encrypted_data
  uint8_t temp_buffer[PADDED_DATA_SIZE];
  memset(temp_buffer, 0, PADDED_DATA_SIZE);
  memcpy(temp_buffer, &ram_cache, sizeof(oath_persist_t));

  bool success =
      aes_encrypt(master_key, flash_data_buffer.iv, temp_buffer,
                  PADDED_DATA_SIZE, flash_data_buffer.encrypted_data);

  if (!success) {
    printf("OATH Storage: ERROR - Encryption failed!\n");
    return;
  }

  // 3. Write the IV + Encrypted Data to flash
  uint32_t ints = save_and_disable_interrupts();
  flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
  flash_range_program(FLASH_TARGET_OFFSET, (const uint8_t *)&flash_data_buffer,
                      sizeof(oath_flash_data_t));
  restore_interrupts(ints);
}

// Helper to load from flash
static void load_from_flash(void) {
  const oath_flash_data_t *stored_flash_data =
      (const oath_flash_data_t *)(XIP_BASE + FLASH_TARGET_OFFSET);

  // Valid magic found, proceed with decryption
  uint8_t master_key[AES_KEY_SIZE_BYTES];
  if (!derive_master_key(master_key)) {
    printf("OATH Storage: ERROR - Could not read Master Key for decryption!\n");
    memset(&ram_cache, 0, sizeof(oath_persist_t));
    return;
  }

  // Decrypt the data from flash into ram_cache
  uint8_t temp_buffer[PADDED_DATA_SIZE];
  bool success = aes_decrypt(master_key, stored_flash_data->iv,
                             stored_flash_data->encrypted_data,
                             PADDED_DATA_SIZE, temp_buffer);

  if (success) {
    memcpy(&ram_cache, temp_buffer, sizeof(oath_persist_t));
    if (ram_cache.magic == 0xDEADBEEF) {
      printf("OATH Storage: Loaded and Decrypted from Flash.\n");
      return;
    }
  }

  printf("OATH Storage: Initializing empty storage.\n");
  memset(&ram_cache, 0, sizeof(oath_persist_t));
  ram_cache.magic = 0xDEADBEEF;
  ram_cache.version = 1;
  save_to_flash();
}

void oath_storage_init(void) { load_from_flash(); }

bool oath_storage_put(const char *name, const uint8_t *secret,
                      uint8_t secret_len, oath_type_t type, oath_algo_t algo,
                      uint8_t digits, uint32_t period, uint8_t touch_required) {

  // Create temporary credential structure
  oath_credential_t temp_cred;
  memset(&temp_cred, 0, sizeof(temp_cred));
  strncpy(temp_cred.name, name, OATH_MAX_NAME_LEN - 1);
  memcpy(temp_cred.secret, secret, secret_len);
  temp_cred.secret_len = secret_len;
  temp_cred.type = type;
  temp_cred.algorithm = algo;
  temp_cred.digits = digits;
  temp_cred.period = period;
  temp_cred.counter = 0;
  temp_cred.touch_required = touch_required;

  // Encrypt the credential
  uint8_t master_key[AES_KEY_SIZE_BYTES];
  if (!derive_master_key(master_key))
    return false;

  encrypted_credential_t encrypted;
  if (!encrypt_credential(master_key, &temp_cred, &encrypted)) {
    return false;
  }

  // Check if update existing
  for (int i = 0; i < MAX_CREDENTIALS; i++) {
    if (ram_cache.slot_used[i]) {
      oath_credential_t existing_cred;
      if (decrypt_credential(master_key, &ram_cache.encrypted_creds[i],
                             &existing_cred)) {
        if (strcmp(existing_cred.name, name) == 0) {
          memcpy(&ram_cache.encrypted_creds[i], &encrypted, sizeof(encrypted));
          save_to_flash();
          return true;
        }
      }
    }
  }

  // Find free slot
  for (int i = 0; i < MAX_CREDENTIALS; i++) {
    if (!ram_cache.slot_used[i]) {
      ram_cache.slot_used[i] = true;
      memcpy(&ram_cache.encrypted_creds[i], &encrypted, sizeof(encrypted));
      save_to_flash();
      return true;
    }
  }

  return false;
}

bool oath_storage_delete(const char *name) {
  uint8_t master_key[AES_KEY_SIZE_BYTES];
  if (!derive_master_key(master_key))
    return false;

  for (int i = 0; i < MAX_CREDENTIALS; i++) {
    if (ram_cache.slot_used[i]) {
      oath_credential_t cred;
      if (decrypt_credential(master_key, &ram_cache.encrypted_creds[i],
                             &cred)) {
        if (strcmp(cred.name, name) == 0) {
          ram_cache.slot_used[i] = false;
          memset(&ram_cache.encrypted_creds[i], 0,
                 sizeof(encrypted_credential_t));
          save_to_flash();
          return true;
        }
      }
    }
  }
  return false;
}

bool oath_storage_get(const char *name, oath_credential_t *out_cred) {
  uint8_t master_key[AES_KEY_SIZE_BYTES];
  if (!derive_master_key(master_key))
    return false;

  for (int i = 0; i < MAX_CREDENTIALS; i++) {
    if (ram_cache.slot_used[i]) {
      oath_credential_t cred;
      if (decrypt_credential(master_key, &ram_cache.encrypted_creds[i],
                             &cred)) {
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

  uint8_t master_key[AES_KEY_SIZE_BYTES];
  if (!derive_master_key(master_key))
    return NULL;

  for (int i = 0; i < MAX_CREDENTIALS; i++) {
    if (ram_cache.slot_used[i]) {
      oath_credential_t cred;
      if (decrypt_credential(master_key, &ram_cache.encrypted_creds[i],
                             &cred)) {
        if (current_idx == index) {
          strncpy(cached_name, cred.name, OATH_MAX_NAME_LEN - 1);
          return cached_name;
        }
        current_idx++;
      }
    }
  }
  return NULL;
}

void oath_storage_reset(void) {
  memset(&ram_cache, 0, sizeof(oath_persist_t));
  ram_cache.magic = 0xDEADBEEF;
  ram_cache.version = 1;
  save_to_flash();
}

bool oath_storage_update_counter(const char *name, uint32_t new_counter) {
  uint8_t master_key[AES_KEY_SIZE_BYTES];
  if (!derive_master_key(master_key))
    return false;

  for (int i = 0; i < MAX_CREDENTIALS; i++) {
    if (ram_cache.slot_used[i]) {
      oath_credential_t cred;
      if (decrypt_credential(master_key, &ram_cache.encrypted_creds[i],
                             &cred)) {
        if (strcmp(cred.name, name) == 0) {
          cred.counter = new_counter;
          encrypted_credential_t encrypted;
          if (encrypt_credential(master_key, &cred, &encrypted)) {
            memcpy(&ram_cache.encrypted_creds[i], &encrypted,
                   sizeof(encrypted));
            save_to_flash();
            return true;
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
  save_to_flash();
  return true;
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
