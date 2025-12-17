#include "hsm.h"
#include "../crypto/aes_gcm.h"
#include "../crypto/uECC.h"
#include "security.h"
#include <hardware/address_mapped.h>
#include <hardware/flash.h>
#include <hardware/sync.h>
#include <pico/rand.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>


// HSM Flash Storage Configuration
// Using the second to last 4KB sector of Flash for HSM data
#define HSM_FLASH_OFFSET (2 * 1024 * 1024 - 8192)
#define HSM_MAGIC 0x48534D21 // "HSM!"

// HSM Slot definitions
typedef struct {
  uint8_t private_key[32];
  uint8_t public_key[64];
  bool occupied;
} hsm_slot_t;

typedef struct {
  uint32_t magic;
  hsm_slot_t slots[HSM_MAX_SLOTS];
} hsm_persist_t;

typedef struct {
  uint8_t iv[12];
  uint8_t encrypted_data[sizeof(hsm_persist_t)];
  uint8_t tag[16];
} hsm_flash_data_t;

static hsm_slot_t hsm_slots[HSM_MAX_SLOTS];

// Internal functions
static void hsm_save_to_flash(void);
static void hsm_load_from_flash(void);

// RNG function for micro-ecc using RP2350 hardware TRNG
static int hsm_rng(uint8_t *dest, unsigned size) {
  while (size >= 4) {
    uint32_t r = get_rand_32();
    memcpy(dest, &r, 4);
    dest += 4;
    size -= 4;
  }
  if (size > 0) {
    uint32_t r = get_rand_32();
    memcpy(dest, &r, size);
  }
  return 1;
}

static void hsm_save_to_flash(void) {
  uint8_t master_key[32];
  if (!otp_read_master_key(master_key)) {
    printf("HSM: ERROR - Could not read Master Key for encryption!\n");
    return;
  }

  hsm_persist_t persist;
  persist.magic = HSM_MAGIC;
  memcpy(persist.slots, hsm_slots, sizeof(hsm_slots));

  hsm_flash_data_t flash_data;
  for (int i = 0; i < 12; i++) {
    flash_data.iv[i] = (uint8_t)(get_rand_32() & 0xFF);
  }

  if (!aes_gcm_encrypt(master_key, flash_data.iv, (const uint8_t *)&persist,
                       sizeof(persist), flash_data.encrypted_data,
                       flash_data.tag)) {
    printf("HSM: ERROR - Flash encryption failed!\n");
    return;
  }

  uint32_t ints = save_and_disable_interrupts();
  flash_range_erase(HSM_FLASH_OFFSET, FLASH_SECTOR_SIZE);
  flash_range_program(HSM_FLASH_OFFSET, (const uint8_t *)&flash_data,
                      sizeof(flash_data));
  restore_interrupts(ints);

  printf("HSM: State persisted to flash.\n");
}

static void hsm_load_from_flash(void) {
  const hsm_flash_data_t *stored_data =
      (const hsm_flash_data_t *)(XIP_BASE + HSM_FLASH_OFFSET);

  uint8_t master_key[32];
  if (!otp_read_master_key(master_key)) {
    printf("HSM: WARNING - No master key, initializing empty HSM.\n");
    memset(hsm_slots, 0, sizeof(hsm_slots));
    return;
  }

  hsm_persist_t persist;
  if (aes_gcm_decrypt(master_key, stored_data->iv, stored_data->encrypted_data,
                      sizeof(persist), stored_data->tag, (uint8_t *)&persist)) {
    if (persist.magic == HSM_MAGIC) {
      memcpy(hsm_slots, persist.slots, sizeof(hsm_slots));
      printf("HSM: Loaded and decrypted from flash.\n");
      return;
    }
  }

  printf("HSM: Initializing empty state.\n");
  memset(hsm_slots, 0, sizeof(hsm_slots));
  hsm_save_to_flash();
}

void hsm_init(void) {
  printf("HSM: Initializing with real ECC (P-256)...\n");

  // Set the RNG function for micro-ecc
  uECC_set_rng(hsm_rng);

  hsm_load_from_flash();
  printf("HSM: Ready with %d slots\n", HSM_MAX_SLOTS);
}

uint8_t hsm_generate_key(uint8_t slot) {
  if (slot >= HSM_MAX_SLOTS)
    return HSM_STATUS_INVALID_SLOT;

  printf("HSM: Generating P-256 key pair for slot %d\n", slot);

  if (!uECC_make_key(hsm_slots[slot].public_key, hsm_slots[slot].private_key,
                     uECC_secp256r1())) {
    printf("HSM: Key generation failed!\n");
    return HSM_STATUS_ERROR;
  }

  hsm_slots[slot].occupied = true;
  hsm_save_to_flash();

  printf("HSM: Key pair generated and saved\n");
  return HSM_STATUS_OK;
}

uint8_t hsm_get_pubkey(uint8_t slot, uint8_t *pubkey_out,
                       uint16_t *pubkey_len) {
  if (slot >= HSM_MAX_SLOTS)
    return HSM_STATUS_INVALID_SLOT;
  if (!hsm_slots[slot].occupied)
    return HSM_STATUS_NO_KEY;

  printf("HSM: Exporting public key for slot %d\n", slot);

  // Return uncompressed public key (X, Y)
  memcpy(pubkey_out, hsm_slots[slot].public_key, 64);
  *pubkey_len = 64;

  return HSM_STATUS_OK;
}

uint8_t hsm_sign(uint8_t slot, const uint8_t *hash, uint8_t *sig_out,
                 uint16_t *sig_len) {
  if (slot >= HSM_MAX_SLOTS)
    return HSM_STATUS_INVALID_SLOT;
  if (!hsm_slots[slot].occupied)
    return HSM_STATUS_NO_KEY;

  printf("HSM: Signing hash with slot %d (ECDSA P-256)\n", slot);

  if (!uECC_sign(hsm_slots[slot].private_key, hash, 32, sig_out,
                 uECC_secp256r1())) {
    printf("HSM: Signing failed!\n");
    return HSM_STATUS_ERROR;
  }

  *sig_len = 64;
  return HSM_STATUS_OK;
}

uint8_t hsm_delete_key(uint8_t slot) {
  if (slot >= HSM_MAX_SLOTS)
    return HSM_STATUS_INVALID_SLOT;

  printf("HSM: Deleting key in slot %d\n", slot);
  memset(&hsm_slots[slot], 0, sizeof(hsm_slot_t));
  hsm_save_to_flash();

  return HSM_STATUS_OK;
}
