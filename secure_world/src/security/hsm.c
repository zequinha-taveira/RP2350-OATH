#include "hsm.h"
#include "../crypto/uECC.h"
#include "pico/rand.h"
#include "pico/stdlib.h"
#include "security.h"
#include <stdio.h>
#include <string.h>

// HSM Slot definitions
typedef struct {
  uint8_t private_key[32];
  uint8_t public_key[64];
  bool occupied;
} hsm_slot_t;

static hsm_slot_t hsm_slots[HSM_MAX_SLOTS];

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

void hsm_init(void) {
  printf("HSM: Initializing with real ECC (P-256)...\n");
  memset(hsm_slots, 0, sizeof(hsm_slots));
  
  // Set the RNG function for micro-ecc
  uECC_set_rng(hsm_rng);
  
  printf("HSM: Ready with %d slots\n", HSM_MAX_SLOTS);
}

uint8_t hsm_generate_key(uint8_t slot) {
  if (slot >= HSM_MAX_SLOTS)
    return HSM_STATUS_INVALID_SLOT;

  printf("HSM: Generating P-256 key pair for slot %d\n", slot);

  if (!uECC_make_key(hsm_slots[slot].public_key, hsm_slots[slot].private_key, uECC_secp256r1())) {
    printf("HSM: Key generation failed!\n");
    return HSM_STATUS_ERROR;
  }

  hsm_slots[slot].occupied = true;

  // TODO: Encrypt and save to flash
  printf("HSM: Key pair generated successfully\n");

  return HSM_STATUS_OK;
}

uint8_t hsm_get_pubkey(uint8_t slot, uint8_t *pubkey_out,
                       uint16_t *pubkey_len) {
  if (slot >= HSM_MAX_SLOTS)
    return HSM_STATUS_INVALID_SLOT;
  if (!hsm_slots[slot].occupied)
    return HSM_STATUS_NO_KEY;

  printf("HSM: Exporting public key for slot %d\n", slot);

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

  if (!uECC_sign(hsm_slots[slot].private_key, hash, 32, sig_out, uECC_secp256r1())) {
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

  // TODO: Update flash

  return HSM_STATUS_OK;
}
