#include "openpgp_storage.h"
#include "../crypto/aes_gcm.h"
#include "../security/security.h"
#include <hardware/address_mapped.h>
#include <hardware/flash.h>
#include <hardware/sync.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>


#define OPENPGP_MAGIC 0x50475033 // "PGP3"

typedef struct {
  uint32_t magic;
  openpgp_data_t data;
  uint8_t iv[12];
  uint8_t tag[16];
  uint8_t encrypted_data[sizeof(openpgp_data_t)];
} openpgp_persist_t;

static openpgp_data_t current_pgp_data;

static void openpgp_save_to_flash(void) {
  uint8_t master_key[32];
  security_get_master_key(master_key);

  openpgp_persist_t persist;
  persist.magic = OPENPGP_MAGIC;
  persist.data = current_pgp_data;

  // Generate random IV (or use simple counter for mock)
  for (int i = 0; i < 12; i++)
    persist.iv[i] = (uint8_t)i;

  if (!aes_gcm_encrypt(master_key, persist.iv, (uint8_t *)&persist.data,
                       sizeof(openpgp_data_t), persist.tag,
                       persist.encrypted_data)) {
    printf("OpenPGP Storage: Encryption failed!\n");
    return;
  }

  uint32_t ints = save_and_disable_interrupts();
  flash_range_erase(OPENPGP_FLASH_OFFSET, FLASH_SECTOR_SIZE);
  flash_range_program(OPENPGP_FLASH_OFFSET, (uint8_t *)&persist,
                      sizeof(persist));
  restore_interrupts(ints);

  printf("OpenPGP Storage: Saved and encrypted to flash.\n");
}

static void openpgp_load_from_flash(void) {
  const openpgp_persist_t *stored_data =
      (const openpgp_persist_t *)(XIP_BASE + OPENPGP_FLASH_OFFSET);

  if (stored_data->magic != OPENPGP_MAGIC) {
    printf("OpenPGP Storage: No valid data in flash. Initializing defaults.\n");
    memset(&current_pgp_data, 0, sizeof(openpgp_data_t));
    memcpy(current_pgp_data.serial, "\x00\x01\x02\x03\x04\x05", 6);
    strcpy(current_pgp_data.name, "RP2350 User");
    current_pgp_data.lang[0] = 'e';
    current_pgp_data.lang[1] = 'n';
    current_pgp_data.pin_retry_counter[0] = 3;
    current_pgp_data.pin_retry_counter[1] = 3;
    current_pgp_data.pin_retry_counter[2] = 3;
    openpgp_save_to_flash();
    return;
  }

  uint8_t master_key[32];
  security_get_master_key(master_key);

  if (aes_gcm_decrypt(master_key, stored_data->iv, stored_data->encrypted_data,
                      sizeof(openpgp_data_t), stored_data->tag,
                      (uint8_t *)&current_pgp_data)) {
    printf("OpenPGP Storage: Loaded and decrypted from flash.\n");
  } else {
    printf("OpenPGP Storage: Decryption failed! Re-initializing.\n");
  }
}

void openpgp_storage_init(void) {
  printf("OpenPGP Storage: Initializing...\n");
  openpgp_load_from_flash();
}

void openpgp_storage_save(void) { openpgp_save_to_flash(); }

openpgp_data_t *openpgp_storage_get_data(void) { return &current_pgp_data; }
