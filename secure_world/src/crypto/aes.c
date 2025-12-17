#include "aes.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

//--------------------------------------------------------------------+
// Stub AES-256 CBC Implementation (NO MBEDTLS)
//--------------------------------------------------------------------+
// WARNING: This is a placeholder because mbedtls headers are missing.
// IT IS NOT SECURE.

// Key length in bits for AES-256
#define AES_KEY_SIZE_BITS 256

/**
 * @brief Simple XOR "encryption" for stubbing.
 */
static void xor_crypt(const uint8_t *key, const uint8_t *iv,
                      const uint8_t *input, size_t len, uint8_t *output) {
  for (size_t i = 0; i < len; i++) {
    output[i] = input[i] ^ key[i % 32] ^ iv[i % 16];
  }
}

/**
 * @brief Encrypts data using AES-256 CBC mode.
 */
bool aes_encrypt(const uint8_t *key, const uint8_t *iv, const uint8_t *input,
                 size_t input_len, uint8_t *output) {
  if (input_len % AES_BLOCK_SIZE != 0) {
    printf("AES_ENC: Error - Input length must be a multiple of block size "
           "(%d).\n",
           AES_BLOCK_SIZE);
    return false;
  }

  // STUB IMPLEMENTATION
  xor_crypt(key, iv, input, input_len, output);
  return true;
}

/**
 * @brief Decrypts data using AES-256 CBC mode.
 */
bool aes_decrypt(const uint8_t *key, const uint8_t *iv, const uint8_t *input,
                 size_t input_len, uint8_t *output) {
  if (input_len % AES_BLOCK_SIZE != 0) {
    printf("AES_DEC: Error - Input length must be a multiple of block size "
           "(%d).\n",
           AES_BLOCK_SIZE);
    return false;
  }

  // STUB IMPLEMENTATION
  xor_crypt(key, iv, input, input_len, output);
  return true;
}
