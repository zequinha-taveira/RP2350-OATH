#include "aes.h"
#include "pico/stdlib.h"
#include "mbedtls/aes.h"
#include "mbedtls/cipher.h" // For padding if needed, but we will assume block size multiple

//--------------------------------------------------------------------+
// AES-256 CBC Implementation using mbedTLS
//--------------------------------------------------------------------+

// Key length in bits for AES-256
#define AES_KEY_SIZE_BITS 256

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
bool aes_encrypt(const uint8_t *key, const uint8_t *iv, const uint8_t *input, size_t input_len, uint8_t *output) {
    if (input_len % AES_BLOCK_SIZE != 0) {
        printf("AES_ENC: Error - Input length must be a multiple of block size (%d).\n", AES_BLOCK_SIZE);
        return false;
    }

    mbedtls_aes_context aes_ctx;
    mbedtls_aes_init(&aes_ctx);
    
    // Set key for encryption
    if (mbedtls_aes_setkey_enc(&aes_ctx, key, AES_KEY_SIZE_BITS) != 0) {
        printf("AES_ENC: Error setting encryption key.\n");
        mbedtls_aes_free(&aes_ctx);
        return false;
    }

    // Copy IV to a mutable buffer as mbedtls_aes_crypt_cbc modifies it
    uint8_t iv_copy[AES_IV_SIZE_BYTES];
    memcpy(iv_copy, iv, AES_IV_SIZE_BYTES);

    // Encrypt in CBC mode
    if (mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_ENCRYPT, input_len, iv_copy, input, output) != 0) {
        printf("AES_ENC: Error during CBC encryption.\n");
        mbedtls_aes_free(&aes_ctx);
        return false;
    }

    mbedtls_aes_free(&aes_ctx);
    return true;
}

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
bool aes_decrypt(const uint8_t *key, const uint8_t *iv, const uint8_t *input, size_t input_len, uint8_t *output) {
    if (input_len % AES_BLOCK_SIZE != 0) {
        printf("AES_DEC: Error - Input length must be a multiple of block size (%d).\n", AES_BLOCK_SIZE);
        return false;
    }

    mbedtls_aes_context aes_ctx;
    mbedtls_aes_init(&aes_ctx);
    
    // Set key for decryption
    if (mbedtls_aes_setkey_dec(&aes_ctx, key, AES_KEY_SIZE_BITS) != 0) {
        printf("AES_DEC: Error setting decryption key.\n");
        mbedtls_aes_free(&aes_ctx);
        return false;
    }

    // Copy IV to a mutable buffer as mbedtls_aes_crypt_cbc modifies it
    uint8_t iv_copy[AES_IV_SIZE_BYTES];
    memcpy(iv_copy, iv, AES_IV_SIZE_BYTES);

    // Decrypt in CBC mode
    if (mbedtls_aes_crypt_cbc(&aes_ctx, MBEDTLS_AES_DECRYPT, input_len, iv_copy, input, output) != 0) {
        printf("AES_DEC: Error during CBC decryption.\n");
        mbedtls_aes_free(&aes_ctx);
        return false;
    }

    mbedtls_aes_free(&aes_ctx);
    return true;
}
