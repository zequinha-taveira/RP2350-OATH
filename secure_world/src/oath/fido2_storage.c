#include "fido2_storage.h"
#include "../crypto/aes_gcm.h"
#include "../security/security.h"
#include <hardware/address_mapped.h>
#include <hardware/flash.h>
#include <hardware/sync.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>


#define FIDO2_MAGIC 0x46444F32 // "FDO2"

typedef struct {
  uint32_t magic;
  fido2_storage_t storage;
  uint8_t iv[12];
  uint8_t tag[16];
  uint8_t encrypted_data[sizeof(fido2_storage_t)];
} fido2_persist_t;

static fido2_storage_t current_fido_storage;

static void fido2_save_to_flash(void) {
  uint8_t master_key[32];
  security_get_master_key(master_key);

  fido2_persist_t persist;
  persist.magic = FIDO2_MAGIC;
  persist.storage = current_fido_storage;

  for (int i = 0; i < 12; i++)
    persist.iv[i] = (uint8_t)i;

  if (!aes_gcm_encrypt(master_key, persist.iv, (uint8_t *)&persist.storage,
                       sizeof(fido2_storage_t), persist.tag,
                       persist.encrypted_data)) {
    printf("FIDO2 Storage: Encryption failed!\n");
    return;
  }

  uint32_t ints = save_and_disable_interrupts();
  flash_range_erase(FIDO2_FLASH_OFFSET, FLASH_SECTOR_SIZE);
  flash_range_program(FIDO2_FLASH_OFFSET, (uint8_t *)&persist, sizeof(persist));
  restore_interrupts(ints);

  printf("FIDO2 Storage: Saved and encrypted to flash.\n");
}

static void fido2_load_from_flash(void) {
  const fido2_persist_t *stored_data =
      (const fido2_persist_t *)(XIP_BASE + FIDO2_FLASH_OFFSET);

  if (stored_data->magic != FIDO2_MAGIC) {
    printf("FIDO2 Storage: No valid data in flash. Initializing empty.\n");
    memset(&current_fido_storage, 0, sizeof(fido2_storage_t));
    fido2_save_to_flash();
    return;
  }

  uint8_t master_key[32];
  security_get_master_key(master_key);

  if (aes_gcm_decrypt(master_key, stored_data->iv, stored_data->encrypted_data,
                      sizeof(fido2_storage_t), stored_data->tag,
                      (uint8_t *)&current_fido_storage)) {
    printf("FIDO2 Storage: Loaded and decrypted from flash.\n");
  } else {
    printf("FIDO2 Storage: Decryption failed! Re-initializing.\n");
    memset(&current_fido_storage, 0, sizeof(fido2_storage_t));
  }
}

void fido2_storage_init(void) {
  printf("FIDO2 Storage: Initializing...\n");
  fido2_load_from_flash();
}

void fido2_storage_save(void) { fido2_save_to_flash(); }

fido2_storage_t *fido2_storage_get_data(void) { return &current_fido_storage; }
