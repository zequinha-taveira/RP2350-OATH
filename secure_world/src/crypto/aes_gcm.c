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
