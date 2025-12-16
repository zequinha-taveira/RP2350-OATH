#ifndef _HMAC_H_
#define _HMAC_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define SHA256_BLOCK_SIZE 64
#define SHA256_DIGEST_SIZE 32

/**
 * @brief Calculates the HMAC-SHA256 of a message using the RP2350 hardware accelerator.
 * 
 * @param key Pointer to the secret key.
 * @param key_len Length of the secret key in bytes.
 * @param data Pointer to the message data.
 * @param data_len Length of the message data in bytes.
 * @param output Pointer to the buffer to store the 32-byte HMAC result.
 * @return true on success, false on failure.
 */
bool hmac_sha256(const uint8_t *key, size_t key_len, const uint8_t *data, size_t data_len, uint8_t *output);

#endif // _HMAC_H_
