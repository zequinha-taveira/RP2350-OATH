#include "oath_storage.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/stdlib.h"
#include "crypto/aes_gcm.h"
#include "security/security.h"
#include <stdio.h>
#include <string.h>

// Flash Storage Configuration
// Using the last 4KB sector of Flash for OATH data
#define FLASH_TARGET_OFFSET (2 * 1024 * 1024 - 4096)
#define FLASH_PAGE_SIZE 256
#define FLASH_SECTOR_SIZE 4096

// Data Structure to persist (encrypted)
typedef struct {
  uint32_t magic; // 0xDEADBEEF
  uint32_t version;
  uint8_t access_code_hash[32]; // SHA-256 hash of PIN
  uint8_t access_code_set; // 0 = not set, 1 = set
  encrypted_credential_t encrypted_creds[MAX_CREDENTIALS];
  bool slot_used[MAX_CREDENTIALS];
  uint8_t master_key_salt[16]; // Salt for key derivation
} oath_persist_t;

static oath_persist_t ram_cache;
static uint8_t master_key[32]; // Cached master key (derived from OTP)

// Helper to derive master key from OTP
static bool derive_master_key(void) {
  // In production, read from OTP
  // For now, generate a stable key based on device ID
  if (!otp_read_master_key(master_key)) {
    // Generate new key if not exists
    if (!otp_write_new_master_key(master_key)) {
      printf("OATH Storage: Failed to generate master key\n");
      return false;
    }
  }
  return true;
}

// Helper to write cache to flash
static void save_to_flash(void) {
  uint32_t ints = save_and_disable_interrupts();
  flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
  flash_range_program(FLASH_TARGET_OFFSET, (const uint8_t *)&ram_cache,
                      sizeof(oath_persist_t));
  restore_interrupts(ints);
  printf("OATH Storage: Persisted encrypted data to Flash at offset 0x%X\n",
         FLASH_TARGET_OFFSET);
}

// Helper to load from flash
static void load_from_flash(void) {
  const uint8_t *flash_ptr = (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET);
  const oath_persist_t *stored_data = (const oath_persist_t *)flash_ptr;

  if (stored_data->magic == 0xDEADBEEF) {
    memcpy(&ram_cache, stored_data, sizeof(oath_persist_t));
    printf("OATH Storage: Loaded encrypted data from Flash.\n");
  } else {
    printf("OATH Storage: No valid flash data found. Initializing empty.\n");
    memset(&ram_cache, 0, sizeof(oath_persist_t));
    ram_cache.magic = 0xDEADBEEF;
    ram_cache.version = 1;
    save_to_flash(); // Init flash
  }
}

void oath_storage_init(void) {
  load_from_flash();
  derive_master_key();
}

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
  encrypted_credential_t encrypted;
  if (!encrypt_credential(master_key, &temp_cred, &encrypted)) {
    printf("OATH Storage: Failed to encrypt credential\n");
    return false;
  }
  
  // Check if update existing
  for (int i = 0; i < MAX_CREDENTIALS; i++) {
    if (ram_cache.slot_used[i]) {
      // Decrypt to check name
      oath_credential_t existing_cred;
      if (decrypt_credential(master_key, &ram_cache.encrypted_creds[i], &existing_cred)) {
        if (strcmp(existing_cred.name, name) == 0) {
          // Update existing
          memcpy(&ram_cache.encrypted_creds[i], &encrypted, sizeof(encrypted));
          save_to_flash();
          printf("OATH Storage: Updated credential '%s' (Touch=%d)\n", name, touch_required);
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
      printf("OATH Storage: Added credential '%s' at slot %d (Touch=%d)\n",
             name, i, touch_required);
      return true;
    }
  }

  printf("OATH Storage: Full!\n");
  return false;
}

bool oath_storage_delete(const char *name) {
  for (int i = 0; i < MAX_CREDENTIALS; i++) {
    if (ram_cache.slot_used[i]) {
      oath_credential_t cred;
      if (decrypt_credential(master_key, &ram_cache.encrypted_creds[i], &cred)) {
        if (strcmp(cred.name, name) == 0) {
          ram_cache.slot_used[i] = false;
          memset(&ram_cache.encrypted_creds[i], 0, sizeof(encrypted_credential_t));
          save_to_flash();
          printf("OATH Storage: Deleted credential '%s'\n", name);
          return true;
        }
      }
    }
  }
  return false;
}

bool oath_storage_get(const char *name, oath_credential_t *out_cred) {
  for (int i = 0; i < MAX_CREDENTIALS; i++) {
    if (ram_cache.slot_used[i]) {
      oath_credential_t cred;
      if (decrypt_credential(master_key, &ram_cache.encrypted_creds[i], &cred)) {
        if (strcmp(cred.name, name) == 0) {
          if (out_cred) {
            memcpy(out_cred, &cred, sizeof(oath_credential_t));
          }
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
  
  for (int i = 0; i < MAX_CREDENTIALS; i++) {
    if (ram_cache.slot_used[i]) {
      oath_credential_t cred;
      if (decrypt_credential(master_key, &ram_cache.encrypted_creds[i], &cred)) {
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
  printf("OATH Storage: Reset all credentials.\n");
}

bool oath_storage_update_counter(const char *name, uint32_t new_counter) {
  for (int i = 0; i < MAX_CREDENTIALS; i++) {
    if (ram_cache.slot_used[i]) {
      oath_credential_t cred;
      if (decrypt_credential(master_key, &ram_cache.encrypted_creds[i], &cred)) {
        if (strcmp(cred.name, name) == 0) {
          cred.counter = new_counter;
          // Re-encrypt and save
          encrypted_credential_t encrypted;
          if (encrypt_credential(master_key, &cred, &encrypted)) {
            memcpy(&ram_cache.encrypted_creds[i], &encrypted, sizeof(encrypted));
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
  if (len > 64) // Max PIN length
    return false;
  
  // Use SHA-256 to hash the PIN
  SHA256_CTX ctx;
  SHA256Init(&ctx);
  SHA256Update(&ctx, code, len);
  SHA256Final(&ctx, ram_cache.access_code_hash);
  
  ram_cache.access_code_set = 1;
  save_to_flash();
  printf("OATH Storage: Access Code Set (Hashed)\n");
  return true;
}

bool oath_storage_verify_password(const uint8_t *code, uint8_t len) {
  if (ram_cache.access_code_set == 0)
    return true; // No password set
  
  uint8_t hash[32];
  SHA256_CTX ctx;
  SHA256Init(&ctx);
  SHA256Update(&ctx, code, len);
  SHA256Final(&ctx, hash);
  
  // Constant time comparison
  volatile uint8_t diff = 0;
  for (int i = 0; i < 32; i++) {
    diff |= hash[i] ^ ram_cache.access_code_hash[i];
  }
  
  return (diff == 0);
}

bool oath_storage_is_password_set(void) {
  return (ram_cache.access_code_set == 1);
}
