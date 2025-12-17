#include "hsm.h"
#include "../crypto/aes_gcm.h"
#include "pico/rand.h"
#include "pico/stdlib.h"
#include "security.h"
#include <stdio.h>
#include <string.h>

// Simulated storage for HSM keys in encrypted flash
// In a real implementation, we would use the hardware-backed flash encryption
// or the same oath_storage mechanism.

typedef struct {
  uint8_t private_key[HSM_KEY_SIZE];
  bool occupied;
} hsm_slot_t;

static hsm_slot_t hsm_slots[HSM_MAX_SLOTS];

void hsm_init(void) {
  printf("HSM: Initializing...\n");
  memset(hsm_slots, 0, sizeof(hsm_slots));
  // In a real device, we would load encrypted keys from flash here.
  printf("HSM: Ready with %d slots\n", HSM_MAX_SLOTS);
}

uint8_t hsm_generate_key(uint8_t slot) {
  if (slot >= HSM_MAX_SLOTS)
    return HSM_STATUS_INVALID_SLOT;

  printf("HSM: Generating key for slot %d\n", slot);

  // Generate 256 bits of random data for the private key
  for (int i = 0; i < HSM_KEY_SIZE / 4; i++) {
    ((uint32_t *)hsm_slots[slot].private_key)[i] = get_rand_32();
  }

  hsm_slots[slot].occupied = true;

  // TODO: Encrypt and save to flash

  return HSM_STATUS_OK;
}

uint8_t hsm_get_pubkey(uint8_t slot, uint8_t *pubkey_out,
                       uint16_t *pubkey_len) {
  if (slot >= HSM_MAX_SLOTS)
    return HSM_STATUS_INVALID_SLOT;
  if (!hsm_slots[slot].occupied)
    return HSM_STATUS_NO_KEY;

  printf("HSM: Exporting public key for slot %d\n", slot);

  // SIMULATION: In a real P-256 implementation, we would derive the public
  // key from the private key. For this project, we'll return a deterministic
  // "public key" based on the private key for testing purposes.
  // Real ECDSA would use point multiplication on the curve.

  memcpy(pubkey_out, hsm_slots[slot].private_key, HSM_KEY_SIZE);
  // Just a dummy "Y" coordinate
  memset(pubkey_out + HSM_KEY_SIZE, 0xEE, HSM_KEY_SIZE);

  *pubkey_len = 64;
  return HSM_STATUS_OK;
}

uint8_t hsm_sign(uint8_t slot, const uint8_t *hash, uint8_t *sig_out,
                 uint16_t *sig_len) {
  if (slot >= HSM_MAX_SLOTS)
    return HSM_STATUS_INVALID_SLOT;
  if (!hsm_slots[slot].occupied)
    return HSM_STATUS_NO_KEY;

  printf("HSM: Signing hash with slot %d\n", slot);

  // SIMULATION: Real ECDSA signing involves (k, G, private key, hash).
  // Here we perform a simple XOR/HMAC-like operation with the private key
  // to simulate a cryptographic attachment for this MVP implementation.

  for (int i = 0; i < 32; i++) {
    sig_out[i] = hash[i] ^ hsm_slots[slot].private_key[i];
  }
  // Dummy "S" component
  memset(sig_out + 32, 0xAA, 32);

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
