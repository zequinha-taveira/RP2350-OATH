#include "oath_storage.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

// RAM-based storage implementation
static oath_credential_t storage[MAX_CREDENTIALS];
static bool slot_used[MAX_CREDENTIALS];

void oath_storage_init(void) {
  memset(storage, 0, sizeof(storage));
  memset(slot_used, 0, sizeof(slot_used));
  printf("OATH Storage: Initialized (RAM)\n");
}

bool oath_storage_put(const char *name, const uint8_t *secret,
                      uint8_t secret_len, oath_type_t type, oath_algo_t algo,
                      uint8_t digits, uint32_t period) {
  // Check if update existing
  for (int i = 0; i < MAX_CREDENTIALS; i++) {
    if (slot_used[i] && strcmp(storage[i].name, name) == 0) {
      // Update
      if (secret_len > OATH_MAX_SECRET_LEN)
        return false;

      memset(&storage[i], 0, sizeof(oath_credential_t));
      strncpy(storage[i].name, name, OATH_MAX_NAME_LEN - 1);
      memcpy(storage[i].secret, secret, secret_len);
      storage[i].secret_len = secret_len;
      storage[i].type = type;
      storage[i].algorithm = algo;
      storage[i].digits = digits;
      storage[i].period = period; // Default usually 30
      storage[i].counter = 0;
      printf("OATH Storage: Updated credential '%s'\n", name);
      return true;
    }
  }

  // Find free slot
  for (int i = 0; i < MAX_CREDENTIALS; i++) {
    if (!slot_used[i]) {
      if (secret_len > OATH_MAX_SECRET_LEN)
        return false;

      slot_used[i] = true;
      strncpy(storage[i].name, name, OATH_MAX_NAME_LEN - 1);
      memcpy(storage[i].secret, secret, secret_len);
      storage[i].secret_len = secret_len;
      storage[i].type = type;
      storage[i].algorithm = algo;
      storage[i].digits = digits;
      storage[i].period = period;
      storage[i].counter = 0;
      printf("OATH Storage: Added credential '%s' at slot %d\n", name, i);
      return true;
    }
  }

  printf("OATH Storage: Full!\n");
  return false;
}

bool oath_storage_delete(const char *name) {
  for (int i = 0; i < MAX_CREDENTIALS; i++) {
    if (slot_used[i] && strcmp(storage[i].name, name) == 0) {
      slot_used[i] = false;
      memset(&storage[i], 0, sizeof(oath_credential_t));
      printf("OATH Storage: Deleted credential '%s'\n", name);
      return true;
    }
  }
  return false;
}

bool oath_storage_get(const char *name, oath_credential_t *out_cred) {
  for (int i = 0; i < MAX_CREDENTIALS; i++) {
    if (slot_used[i] && strcmp(storage[i].name, name) == 0) {
      if (out_cred) {
        memcpy(out_cred, &storage[i], sizeof(oath_credential_t));
      }
      return true;
    }
  }
  return false;
}

const char *oath_storage_list(uint32_t index) {
  uint32_t current_idx = 0;
  for (int i = 0; i < MAX_CREDENTIALS; i++) {
    if (slot_used[i]) {
      if (current_idx == index) {
        return storage[i].name;
      }
      current_idx++;
    }
  }
  return NULL;
}

void oath_storage_reset(void) {
  memset(storage, 0, sizeof(storage));
  memset(slot_used, 0, sizeof(slot_used));
  printf("OATH Storage: Reset all credentials.\n");
}

bool oath_storage_update_counter(const char *name, uint32_t new_counter) {
  for (int i = 0; i < MAX_CREDENTIALS; i++) {
    if (slot_used[i] && strcmp(storage[i].name, name) == 0) {
      storage[i].counter = new_counter;
      return true;
    }
  }
  return false;
}
