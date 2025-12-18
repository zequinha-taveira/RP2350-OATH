#include "aes_gcm.h"
#include "aes.h"
#include <string.h>

/**
 * @file aes_gcm.c
 * @brief Software implementation of AES-GCM 256.
 */

// GHASH implementation (simplified for 128-bit blocks)
static void gmult(uint8_t *x, const uint8_t *h) {
  uint8_t z[16] = {0};
  uint8_t v[16];
  memcpy(v, h, 16);

  for (int i = 0; i < 128; i++) {
    if ((x[i >> 3] >> (7 - (i & 7))) & 1) {
      for (int j = 0; j < 16; j++)
        z[j] ^= v[j];
    }
    uint8_t last_bit = v[15] & 1;
    for (int j = 15; j > 0; j--) {
      v[j] = (v[j] >> 1) | (v[j - 1] << 7);
    }
    v[0] >>= 1;
    if (last_bit)
      v[0] ^= 0xE1;
  }
  memcpy(x, z, 16);
}

static void ghash(const uint8_t *h, const uint8_t *data, size_t len,
                  uint8_t *y) {
  uint8_t b[16];
  memset(y, 0, 16);
  for (size_t i = 0; i < len; i += 16) {
    size_t n = (len - i) > 16 ? 16 : (len - i);
    memset(b, 0, 16);
    memcpy(b, data + i, n);
    for (int j = 0; j < 16; j++)
      y[j] ^= b[j];
    gmult(y, h);
  }
}

// AES-CTR implementation for GCM
static void aes_ctr_xcrypt(const uint8_t *w, uint8_t *cb, uint8_t *data,
                           size_t len) {
  uint8_t keystream[16];
  for (size_t i = 0; i < len; i += 16) {
    aes_ecb_encrypt_block(cb, w, keystream);
    size_t n = (len - i) > 16 ? 16 : (len - i);
    for (size_t j = 0; j < n; j++)
      data[i + j] ^= keystream[j];

    // Increment counter (32-bit big-endian at the end of CB)
    for (int j = 15; j >= 12; j--) {
      if (++cb[j])
        break;
    }
  }
}

bool aes_gcm_encrypt(const uint8_t *key, const uint8_t *iv,
                     const uint8_t *plaintext, size_t plaintext_len,
                     uint8_t *ciphertext, uint8_t *tag) {
  uint8_t w[240];
  uint8_t h[16] = {0};
  uint8_t cb[16];
  uint8_t j0[16];

  aes_key_expansion(key, w);
  aes_ecb_encrypt_block(h, w, h); // H = E(K, 0^128)

  // J0 = IV || 0^31 || 1 (assuming 96-bit IV)
  memcpy(j0, iv, 12);
  memset(j0 + 12, 0, 3);
  j0[15] = 1;

  memcpy(cb, j0, 16);
  // Increment for first block of data
  for (int j = 15; j >= 12; j--) {
    if (++cb[j])
      break;
  }

  memcpy(ciphertext, plaintext, plaintext_len);
  aes_ctr_xcrypt(w, cb, ciphertext, plaintext_len);

  // Compute Tag
  uint8_t y[16];
  uint8_t len_block[16] = {0};
  uint64_t bit_len = (uint64_t)plaintext_len * 8;
  // Assuming AAD is empty for now as per current use case
  ghash(h, ciphertext, plaintext_len, y);

  // Final block with lengths (AAD len, Ciphertext len)
  // For now OATH storage doesn't use AAD
  for (int i = 0; i < 8; i++)
    len_block[15 - i] = (uint8_t)(bit_len >> (i * 8));
  for (int j = 0; j < 16; j++)
    y[j] ^= len_block[j];
  gmult(y, h);

  uint8_t t0[16];
  aes_ecb_encrypt_block(j0, w, t0);
  for (int i = 0; i < 16; i++)
    tag[i] = y[i] ^ t0[i];

  return true;
}

bool aes_gcm_decrypt(const uint8_t *key, const uint8_t *iv,
                     const uint8_t *ciphertext, size_t ciphertext_len,
                     const uint8_t *tag, uint8_t *plaintext) {
  uint8_t w[240];
  uint8_t h[16] = {0};
  uint8_t j0[16];
  uint8_t expected_tag[16];

  aes_key_expansion(key, w);
  aes_ecb_encrypt_block(h, w, h);

  memcpy(j0, iv, 12);
  memset(j0 + 12, 0, 3);
  j0[15] = 1;

  // Verify tag before decryption (encrypt ciphertext and check tag)
  uint8_t y[16];
  uint8_t len_block[16] = {0};
  uint64_t bit_len = (uint64_t)ciphertext_len * 8;
  ghash(h, ciphertext, ciphertext_len, y);
  for (int i = 0; i < 8; i++)
    len_block[15 - i] = (uint8_t)(bit_len >> (i * 8));
  for (int j = 0; j < 16; j++)
    y[j] ^= len_block[j];
  gmult(y, h);

  uint8_t t0[16];
  aes_ecb_encrypt_block(j0, w, t0);
  for (int i = 0; i < 16; i++)
    expected_tag[i] = y[i] ^ t0[i];

  // Constant time comparison
  uint8_t diff = 0;
  for (int i = 0; i < 16; i++)
    diff |= (tag[i] ^ expected_tag[i]);
  if (diff != 0)
    return false;

  // Decrypt
  uint8_t cb[16];
  memcpy(cb, j0, 16);
  for (int j = 15; j >= 12; j--) {
    if (++cb[j])
      break;
  }

  memcpy(plaintext, ciphertext, ciphertext_len);
  aes_ctr_xcrypt(w, cb, plaintext, ciphertext_len);

  return true;
}
