#include "aes_gcm.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

// Simulated AES-GCM implementation
// Note: In production, use hardware crypto accelerator or mbedTLS

/**
 * @brief Generate a random IV (Initialization Vector)
 */
static void generate_iv(uint8_t *iv, size_t iv_len) {
    // Use simple pseudo-random for simulation
    // In production, use hardware TRNG
    for (size_t i = 0; i < iv_len; i++) {
        iv[i] = (uint8_t)(time_us_32() & 0xFF) ^ (i * 0x55);
    }
}

/**
 * @brief XOR operation for simple encryption
 * @param key Encryption key (32 bytes for AES-256)
 * @param iv Initialization vector (12 bytes)
 * @param data Data to encrypt/decrypt
 * @param data_len Length of data
 * @param output Output buffer
 */
static void simple_xor_crypt(const uint8_t *key, const uint8_t *iv, 
                            const uint8_t *data, size_t data_len, 
                            uint8_t *output) {
    // Simple XOR-based encryption (NOT SECURE - for demonstration only)
    // In production, use real AES-GCM with hardware acceleration
    
    for (size_t i = 0; i < data_len; i++) {
        uint8_t key_byte = key[i % 32];
        uint8_t iv_byte = iv[i % 12];
        output[i] = data[i] ^ key_byte ^ iv_byte;
    }
}

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
                    uint8_t *ciphertext, uint8_t *tag) {
    if (!key || !iv || !plaintext || !ciphertext || !tag) {
        return false;
    }
    
    // Encrypt data
    simple_xor_crypt(key, iv, plaintext, plaintext_len, ciphertext);
    
    // Generate authentication tag (simplified)
    // In real AES-GCM, this would be computed using GHASH
    for (int i = 0; i < 16; i++) {
        tag[i] = ciphertext[i % plaintext_len] ^ key[i] ^ iv[i % 12];
    }
    
    return true;
}

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
                    const uint8_t *tag, uint8_t *plaintext) {
    if (!key || !iv || !ciphertext || !tag || !plaintext) {
        return false;
    }
    
    // Decrypt data
    simple_xor_crypt(key, iv, ciphertext, ciphertext_len, plaintext);
    
    // Verify tag (simplified)
    uint8_t computed_tag[16];
    for (int i = 0; i < 16; i++) {
        computed_tag[i] = ciphertext[i % ciphertext_len] ^ key[i] ^ iv[i % 12];
    }
    
    // Constant-time comparison
    bool tag_match = true;
    for (int i = 0; i < 16; i++) {
        if (computed_tag[i] != tag[i]) {
            tag_match = false;
        }
    }
    
    return tag_match;
}

/**
 * @brief Encrypt credential structure
 * @param key Master encryption key
 * @param cred Credential to encrypt
 * @param encrypted Output encrypted structure
 * @return true if successful
 */
bool encrypt_credential(const uint8_t *key, const oath_credential_t *cred,
                       encrypted_credential_t *encrypted) {
    uint8_t iv[12];
    generate_iv(iv, sizeof(iv));
    
    // Copy IV to output
    memcpy(encrypted->iv, iv, sizeof(iv));
    
    // Encrypt the credential data (excluding name for indexing)
    // We encrypt: secret, secret_len, type, algorithm, digits, counter, period, touch_required
    uint8_t plaintext[64]; // Enough for all fields
    size_t plaintext_len = 0;
    
    memcpy(plaintext + plaintext_len, &cred->secret_len, 1);
    plaintext_len += 1;
    memcpy(plaintext + plaintext_len, cred->secret, cred->secret_len);
    plaintext_len += cred->secret_len;
    memcpy(plaintext + plaintext_len, &cred->type, 1);
    plaintext_len += 1;
    memcpy(plaintext + plaintext_len, &cred->algorithm, 1);
    plaintext_len += 1;
    memcpy(plaintext + plaintext_len, &cred->digits, 1);
    plaintext_len += 1;
    memcpy(plaintext + plaintext_len, &cred->counter, 4);
    plaintext_len += 4;
    memcpy(plaintext + plaintext_len, &cred->period, 4);
    plaintext_len += 4;
    memcpy(plaintext + plaintext_len, &cred->touch_required, 1);
    plaintext_len += 1;
    
    // Encrypt
    if (!aes_gcm_encrypt(key, iv, plaintext, plaintext_len, 
                        encrypted->ciphertext, encrypted->tag)) {
        return false;
    }
    
    encrypted->ciphertext_len = plaintext_len;
    return true;
}

/**
 * @brief Decrypt credential structure
 * @param key Master encryption key
 * @param encrypted Encrypted structure
 * @param cred Output decrypted credential
 * @return true if successful
 */
bool decrypt_credential(const uint8_t *key, 
                       const encrypted_credential_t *encrypted,
                       oath_credential_t *cred) {
    uint8_t plaintext[64];
    
    // Decrypt
    if (!aes_gcm_decrypt(key, encrypted->iv, 
                        encrypted->ciphertext, encrypted->ciphertext_len,
                        encrypted->tag, plaintext)) {
        return false;
    }
    
    // Parse decrypted data
    size_t offset = 0;
    memcpy(&cred->secret_len, plaintext + offset, 1);
    offset += 1;
    memcpy(cred->secret, plaintext + offset, cred->secret_len);
    offset += cred->secret_len;
    memcpy(&cred->type, plaintext + offset, 1);
    offset += 1;
    memcpy(&cred->algorithm, plaintext + offset, 1);
    offset += 1;
    memcpy(&cred->digits, plaintext + offset, 1);
    offset += 1;
    memcpy(&cred->counter, plaintext + offset, 4);
    offset += 4;
    memcpy(&cred->period, plaintext + offset, 4);
    offset += 4;
    memcpy(&cred->touch_required, plaintext + offset, 1);
    
    return true;
}