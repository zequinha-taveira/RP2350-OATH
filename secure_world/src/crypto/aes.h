#ifndef _AES_H_
#define _AES_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// We will use AES-256 CBC mode
#define AES_KEY_SIZE_BYTES 32
#define AES_IV_SIZE_BYTES 16
#define AES_BLOCK_SIZE 16

/**
 * @brief Encrypts data using AES-256 CBC mode.
 * 
 * @param key 32-byte encryption key (Master Key from OTP).
 * @param iv 16-byte Initialization Vector (must be unique for each encryption).
 * @param input Pointer to the data to encrypt.
 * @param input_len Length of the data to encrypt (must be a multiple of AES_BLOCK_SIZE).
 * @param output Pointer to the buffer to store the encrypted data.
 * @return true on success, false on failure.
 */
bool aes_encrypt(const uint8_t *key, const uint8_t *iv, const uint8_t *input, size_t input_len, uint8_t *output);

/**
 * @brief Decrypts data using AES-256 CBC mode.
 * 
 * @param key 32-byte decryption key (Master Key from OTP).
 * @param iv 16-byte Initialization Vector.
 * @param input Pointer to the data to decrypt.
 * @param input_len Length of the data to decrypt (must be a multiple of AES_BLOCK_SIZE).
 * @param output Pointer to the buffer to store the decrypted data.
 * @return true on success, false on failure.
 */
bool aes_decrypt(const uint8_t *key, const uint8_t *iv, const uint8_t *input, size_t input_len, uint8_t *output);

#endif // _AES_H_
