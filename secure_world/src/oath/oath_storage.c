#include "oath_storage.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include "security/security.h"
#include "crypto/aes.h"
#include "pico/rand.h" // Assuming this provides a basic TRNG/RNG for IV, will replace with proper TRNG if available
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

// Flash Storage Configuration
// Using the last 4KB sector of Flash for OATH data
// Adjust based on your flash size (e.g., 2MB = 0x200000)
// For RP2350 with typically 2MB+, we target (FLASH_SIZE_BYTES - 4096)
// We'll define a safe default for 2MB, but this should be board specific.
#define FLASH_TARGET_OFFSET (2 * 1024 * 1024 - 4096)
#define FLASH_PAGE_SIZE 256
#define FLASH_SECTOR_SIZE 4096

// Calculate padded size for encryption
#define PADDED_DATA_SIZE ((sizeof(oath_persist_t) + AES_BLOCK_SIZE - 1) / AES_BLOCK_SIZE) * AES_BLOCK_SIZE

// Structure to be written to flash (IV + Encrypted Data)
typedef struct {
  uint8_t iv[AES_IV_SIZE_BYTES];
  uint8_t encrypted_data[PADDED_DATA_SIZE];
} oath_flash_data_t;

// Data Structure to persist
typedef struct {
  uint32_t magic; // 0xDEADBEEF
  uint32_t version;
  uint8_t access_code[OATH_MAX_SECRET_LEN];
  uint8_t access_code_len;
  oath_credential_t credentials[MAX_CREDENTIALS];
  bool slot_used[MAX_CREDENTIALS];
} oath_persist_t;

static oath_persist_t ram_cache;
static oath_flash_data_t flash_data_buffer; // Buffer for encryption/decryption

// Helper to write cache to flash
static void save_to_flash(void) {
  uint8_t master_key[AES_KEY_SIZE_BYTES];
  if (!otp_read_master_key(master_key)) {
    printf("OATH Storage: ERROR - Could not read Master Key for encryption!\n");
    return;
  }

  // 1. Generate a new random IV
  // Note: get_rand_32() is a placeholder. A true TRNG should be used.
  for (int i = 0; i < AES_IV_SIZE_BYTES / 4; i++) {
    ((uint32_t *)flash_data_buffer.iv)[i] = get_rand_32();
  }

  // 2. Apply zero-padding to ram_cache up to PADDED_DATA_SIZE
  size_t struct_size = sizeof(oath_persist_t);
  if (PADDED_DATA_SIZE > struct_size) {
    memset(((uint8_t *)&ram_cache) + struct_size, 0, PADDED_DATA_SIZE - struct_size);
  }

  // 3. Encrypt ram_cache into flash_data_buffer.encrypted_data
  bool success = aes_encrypt(
      master_key,
      flash_data_buffer.iv,
      (const uint8_t *)&ram_cache,
      PADDED_DATA_SIZE, // Use padded size for encryption
      flash_data_buffer.encrypted_data);

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
  printf("OATH Storage: Persisted (Encrypted) to Flash at offset 0x%X\n",
         FLASH_TARGET_OFFSET);
}

// Helper to load from flash
static void load_from_flash(void) {
  const oath_flash_data_t *stored_flash_data =
      (const oath_flash_data_t *)(XIP_BASE + FLASH_TARGET_OFFSET);

  // Check if the first 4 bytes of the encrypted data sector are the magic number
  // This is a weak check, but necessary before decryption
  const oath_persist_t *stored_data_header =
      (const oath_persist_t *)stored_flash_data->encrypted_data;

  if (stored_data_header->magic != 0xDEADBEEF) {
    printf("OATH Storage: No valid flash data found (Magic check failed). Initializing empty.\n");
    memset(&ram_cache, 0, sizeof(oath_persist_t));
    ram_cache.magic = 0xDEADBEEF;
    save_to_flash(); // Init flash (will encrypt and write)
    return;
  }

  // Valid magic found, proceed with decryption
  uint8_t master_key[AES_KEY_SIZE_BYTES];
  if (!otp_read_master_key(master_key)) {
    printf("OATH Storage: ERROR - Could not read Master Key for decryption!\n");
    // Cannot proceed without key, keep ram_cache empty
    memset(&ram_cache, 0, sizeof(oath_persist_t));
    return;
  }

  // Decrypt the data from flash into ram_cache
  bool success = aes_decrypt(
      master_key,
      stored_flash_data->iv,
      stored_flash_data->encrypted_data,
      PADDED_DATA_SIZE, // Use padded size for decryption
      (uint8_t *)&ram_cache);

  if (success && ram_cache.magic == 0xDEADBEEF) {
    printf("OATH Storage: Loaded and Decrypted from Flash.\n");
  } else {
    printf("OATH Storage: ERROR - Decryption failed or data corrupted. Initializing empty.\n");
    memset(&ram_cache, 0, sizeof(oath_persist_t));
    ram_cache.magic = 0xDEADBEEF;
    save_to_flash(); // Overwrite corrupted data
  }
}

void oath_storage_init(void) { load_from_flash(); }

bool oath_storage_put(const char *name, const uint8_t *secret,
                      uint8_t secret_len, oath_type_t type, oath_algo_t algo,
                      uint8_t digits, uint32_t period, uint8_t touch_required) {
  // Check if update existing
  for (int i = 0; i < MAX_CREDENTIALS; i++) {
    if (ram_cache.slot_used[i] &&
        strcmp(ram_cache.credentials[i].name, name) == 0) {
      // Update
      if (secret_len > OATH_MAX_SECRET_LEN)
        return false;

      memset(&ram_cache.credentials[i], 0, sizeof(oath_credential_t));
      strncpy(ram_cache.credentials[i].name, name, OATH_MAX_NAME_LEN - 1);
      memcpy(ram_cache.credentials[i].secret, secret, secret_len);
      ram_cache.credentials[i].secret_len = secret_len;
      ram_cache.credentials[i].type = type;
      ram_cache.credentials[i].algorithm = algo;
      ram_cache.credentials[i].digits = digits;
      ram_cache.credentials[i].period = period; // Default usually 30
      ram_cache.credentials[i].counter = 0;
      ram_cache.credentials[i].touch_required = touch_required;

      save_to_flash();
      printf("OATH Storage: Updated credential '%s' (Touch=%d)\n", name,
             touch_required);
      return true;
    }
  }

  // Find free slot
  for (int i = 0; i < MAX_CREDENTIALS; i++) {
    if (!ram_cache.slot_used[i]) {
      if (secret_len > OATH_MAX_SECRET_LEN)
        return false;

      ram_cache.slot_used[i] = true;
      strncpy(ram_cache.credentials[i].name, name, OATH_MAX_NAME_LEN - 1);
      memcpy(ram_cache.credentials[i].secret, secret, secret_len);
      ram_cache.credentials[i].secret_len = secret_len;
      ram_cache.credentials[i].type = type;
      ram_cache.credentials[i].algorithm = algo;
      ram_cache.credentials[i].digits = digits;
      ram_cache.credentials[i].period = period;
      ram_cache.credentials[i].counter = 0;
      ram_cache.credentials[i].touch_required = touch_required;

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
    if (ram_cache.slot_used[i] &&
        strcmp(ram_cache.credentials[i].name, name) == 0) {
      ram_cache.slot_used[i] = false;
      memset(&ram_cache.credentials[i], 0, sizeof(oath_credential_t));
      save_to_flash();
      printf("OATH Storage: Deleted credential '%s'\n", name);
      return true;
    }
  }
  return false;
}

bool oath_storage_get(const char *name, oath_credential_t *out_cred) {
  for (int i = 0; i < MAX_CREDENTIALS; i++) {
    if (ram_cache.slot_used[i] &&
        strcmp(ram_cache.credentials[i].name, name) == 0) {
      if (out_cred) {
        memcpy(out_cred, &ram_cache.credentials[i], sizeof(oath_credential_t));
      }
      return true;
    }
  }
  return false;
}

const char *oath_storage_list(uint32_t index) {
  uint32_t current_idx = 0;
  for (int i = 0; i < MAX_CREDENTIALS; i++) {
    if (ram_cache.slot_used[i]) {
      if (current_idx == index) {
        return ram_cache.credentials[i].name;
      }
      current_idx++;
    }
  }
  return NULL;
}

void oath_storage_reset(void) {
  memset(&ram_cache, 0, sizeof(oath_persist_t));
  ram_cache.magic = 0xDEADBEEF;
  save_to_flash();
  printf("OATH Storage: Reset all credentials.\n");
}

bool oath_storage_update_counter(const char *name, uint32_t new_counter) {
  for (int i = 0; i < MAX_CREDENTIALS; i++) {
    if (ram_cache.slot_used[i] &&
        strcmp(ram_cache.credentials[i].name, name) == 0) {
      ram_cache.credentials[i].counter = new_counter;
      save_to_flash();
      return true;
    }
  }
  return false;
}

bool oath_storage_set_password(const uint8_t *code, uint8_t len) {
  if (len > OATH_MAX_SECRET_LEN)
    return false;
  // In production, PBKDF2 or similar should be used here.
  // For now, raw copy for simulation.
  memcpy(ram_cache.access_code, code, len);
  ram_cache.access_code_len = len;
  save_to_flash();
  printf("OATH Storage: Access Code Set (Len=%d)\n", len);
  return true;
}

bool oath_storage_verify_password(const uint8_t *code, uint8_t len) {
  if (ram_cache.access_code_len == 0)
    return true; // No password set? Logic error if called here, but usually
                 // true means "Auth OK"
  if (len != ram_cache.access_code_len)
    return false;
  // Constant time compare ideally
  return (memcmp(ram_cache.access_code, code, len) == 0);
}

bool oath_storage_is_password_set(void) {
  return (ram_cache.access_code_len > 0);
}
