#ifndef HSM_H
#define HSM_H

#include <stdbool.h>
#include <stdint.h>


// HSM Slot definitions
#define HSM_MAX_SLOTS 8
#define HSM_KEY_SIZE 32 // 256-bit keys (e.g., P-256)

// HSM Status codes
#define HSM_STATUS_OK 0x00
#define HSM_STATUS_ERROR 0x01
#define HSM_STATUS_INVALID_SLOT 0x02
#define HSM_STATUS_NO_KEY 0x03

// HSM Initialize
void hsm_init(void);

// Generate a new key in a slot
// Returns HSM_STATUS_OK on success
uint8_t hsm_generate_key(uint8_t slot);

// Get the public key for a slot
// pubkey_out must be at least 64 bytes (X and Y coordinates for P-256)
uint8_t hsm_get_pubkey(uint8_t slot, uint8_t *pubkey_out, uint16_t *pubkey_len);

// Sign a hash with the key in a slot
// hash must be 32 bytes (SHA-256)
// sig_out must be at least 64 bytes (R and S components)
uint8_t hsm_sign(uint8_t slot, const uint8_t *hash, uint8_t *sig_out,
                 uint16_t *sig_len);

// Delete a key in a slot
uint8_t hsm_delete_key(uint8_t slot);

#endif // HSM_H
