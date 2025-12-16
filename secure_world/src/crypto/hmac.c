#include "hmac.h"
// #include "pico/sha256_hw.h" // Missing in SDK, stubbing
#include "libcotp/src/whmac.h"
#include <stdio.h>
#include <string.h>

// Dummy context for build
typedef struct {
  uint8_t buffer[64];
} sha256_hw_context_t;

// Dummy functions
static void sha256_hw_init(sha256_hw_context_t *ctx) {}
static void sha256_hw_update(sha256_hw_context_t *ctx, const uint8_t *data,
                             size_t len) {}
static void sha256_hw_final(sha256_hw_context_t *ctx, uint8_t *output) {
  // Return dummy hash
  memset(output, 0xCC, 32);
}

// Global ctx
static sha256_hw_context_t hw_sha256_ctx;

static void hardware_sha256(const uint8_t *data, size_t len, uint8_t *output) {
  printf("CRYPTO: Using DUMMY SHA-256 (Stub).\n");
  sha256_hw_init(&hw_sha256_ctx);
  sha256_hw_update(&hw_sha256_ctx, data, len);
  sha256_hw_final(&hw_sha256_ctx, output);
}

// Inner and outer padded keys
static uint8_t k_ipad[64];
static uint8_t k_opad[64];

#define SHA256_BLOCK_SIZE 64
#define SHA256_DIGEST_SIZE 32

int whmac_sha256(const uint8_t *key, size_t key_len, const uint8_t *data,
                 size_t data_len, uint8_t *output) {
  sha256_hw_context_t ctx;
  uint8_t temp_hash[SHA256_DIGEST_SIZE];

  if (key_len > SHA256_BLOCK_SIZE) {
    sha256_hw_init(&ctx);
    sha256_hw_update(&ctx, key, key_len);
    sha256_hw_final(&ctx, temp_hash);
    key = temp_hash;
    key_len = SHA256_DIGEST_SIZE;
  }

  memset(k_ipad, 0, SHA256_BLOCK_SIZE);
  memset(k_opad, 0, SHA256_BLOCK_SIZE);
  memcpy(k_ipad, key, key_len);
  memcpy(k_opad, key, key_len);

  for (int i = 0; i < SHA256_BLOCK_SIZE; i++) {
    k_ipad[i] ^= 0x36;
    k_opad[i] ^= 0x5C;
  }

  // Inner
  sha256_hw_init(&ctx);
  sha256_hw_update(&ctx, k_ipad, SHA256_BLOCK_SIZE);
  sha256_hw_update(&ctx, data, data_len);
  sha256_hw_final(&ctx, temp_hash);

  // Outer
  sha256_hw_init(&ctx);
  sha256_hw_update(&ctx, k_opad, SHA256_BLOCK_SIZE);
  sha256_hw_update(&ctx, temp_hash, SHA256_DIGEST_SIZE);
  sha256_hw_final(&ctx, output);

  return 0;
}

int whmac_sha1(const uint8_t *key, size_t key_len, const uint8_t *data,
               size_t data_len, uint8_t *output) {
  printf("CRYPTO: Using software HMAC-SHA1 (Stub)...\n");
  return 0;
}
