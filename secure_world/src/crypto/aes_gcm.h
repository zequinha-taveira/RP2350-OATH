#ifndef AES_GCM_H
#define AES_GCM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


/**
 * @brief Encrypt data using AES-GCM
 * @param key Encryption key (32 bytes for AES-256)
 * @param iv Initialization vector (12 bytes)
 * @param plaintext Data to encrypt
 * @param plaintext_len Length of plaintext
 * @param ciphertext Output buffer for encrypted data
 * @param tag Output buffer for authentication tag (16 bytes)
 * @return true if successful
 */
bool aes_gcm_encrypt(const uint8_t *key, const uint8_t *iv,
                     const uint8_t *plaintext, size_t plaintext_len,
                     uint8_t *ciphertext, uint8_t *tag);

/**
 * @brief Decrypt data using AES-GCM
 * @param key Encryption key (32 bytes for AES-256)
 * @param iv Initialization vector (12 bytes)
 * @param ciphertext Encrypted data
 * @param ciphertext_len Length of ciphertext
 * @param tag Authentication tag (16 bytes)
 * @param plaintext Output buffer for decrypted data
 * @return true if successful and tag matches
 */
bool aes_gcm_decrypt(const uint8_t *key, const uint8_t *iv,
                     const uint8_t *ciphertext, size_t ciphertext_len,
                     const uint8_t *tag, uint8_t *plaintext);

#endif // AES_GCM_H