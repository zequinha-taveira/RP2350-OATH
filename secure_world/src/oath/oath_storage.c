#include "oath_storage.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
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

// Helper to write cache to flash
static void save_to_flash(void) {
  uint32_t ints = save_and_disable_interrupts();
  flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
  flash_range_program(FLASH_TARGET_OFFSET, (const uint8_t *)&ram_cache,
                      sizeof(oath_persist_t));
  restore_interrupts(ints);
  printf("OATH Storage: Persisted to Flash at offset 0x%X\n",
         FLASH_TARGET_OFFSET);
}

// Helper to load from flash
static void load_from_flash(void) {
  const uint8_t *flash_ptr = (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET);
  const oath_persist_t *stored_data = (const oath_persist_t *)flash_ptr;

  if (stored_data->magic == 0xDEADBEEF) {
    memcpy(&ram_cache, stored_data, sizeof(oath_persist_t));
    printf("OATH Storage: Loaded from Flash.\n");
  } else {
    printf("OATH Storage: No valid flash data found. Initializing empty.\n");
    memset(&ram_cache, 0, sizeof(oath_persist_t));
    ram_cache.magic = 0xDEADBEEF;
    save_to_flash(); // Init flash
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
