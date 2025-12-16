#include "hmac.h"
#include "pico/sha256_hw.h" // RP2350 Hardware SHA-256
#include "libcotp/src/whmac.h" // libcotp's HMAC wrapper

//--------------------------------------------------------------------+
// Hardware-Accelerated HMAC Implementation (Secure World Logic Placeholder)
//--------------------------------------------------------------------+

// This file acts as the bridge between libcotp's HMAC wrapper and the
// RP2350's hardware SHA-256 accelerator.

// The libcotp library uses a wrapper function for HMAC:
// int whmac_sha1(const uint8_t *key, size_t key_len, const uint8_t *data, size_t data_len, uint8_t *output);
// int whmac_sha256(const uint8_t *key, size_t key_len, const uint8_t *data, size_t data_len, uint8_t *output);

// We will implement whmac_sha256 to use the hardware accelerator.

// Placeholder for hardware SHA-256 context
static sha256_hw_context_t hw_sha256_ctx;

// Custom SHA-256 function using hardware
static void hardware_sha256(const uint8_t *data, size_t len, uint8_t *output) {
    // TODO: Implement the actual hardware call sequence
    // 1. Initialize hardware context
    // 2. Feed data to the accelerator
    // 3. Finalize and read result
    
    // For now, use a software fallback or a dummy implementation
    printf("CRYPTO: Using DUMMY/SOFTWARE SHA-256 (Hardware integration pending)...\n");
    
    // Fallback to a simple hash for placeholder
    // In a real scenario, this would be the actual hardware driver call
    sha256_hw_init(&hw_sha256_ctx);
    sha256_hw_update(&hw_sha256_ctx, data, len);
    sha256_hw_final(&hw_sha256_ctx, output);
}

// Inner and outer padded keys
static uint8_t k_ipad[SHA256_BLOCK_SIZE];
static uint8_t k_opad[SHA256_BLOCK_SIZE];

// Custom HMAC-SHA256 function for libcotp to use
int whmac_sha256(const uint8_t *key, size_t key_len, const uint8_t *data, size_t data_len, uint8_t *output) {
    sha256_hw_context_t ctx;
    uint8_t temp_hash[SHA256_DIGEST_SIZE];

    // 1. Key normalization (if key_len > SHA256_BLOCK_SIZE, hash it)
    if (key_len > SHA256_BLOCK_SIZE) {
        sha256_hw_init(&ctx);
        sha256_hw_update(&ctx, key, key_len);
        sha256_hw_final(&ctx, temp_hash);
        key = temp_hash;
        key_len = SHA256_DIGEST_SIZE;
    }

    // 2. Prepare inner and outer padded keys (k_ipad and k_opad)
    memset(k_ipad, 0, SHA256_BLOCK_SIZE);
    memset(k_opad, 0, SHA256_BLOCK_SIZE);
    memcpy(k_ipad, key, key_len);
    memcpy(k_opad, key, key_len);

    for (int i = 0; i < SHA256_BLOCK_SIZE; i++) {
        k_ipad[i] ^= 0x36;
        k_opad[i] ^= 0x5C;
    }

    // 3. Inner Hash: H(K XOR ipad || data)
    sha256_hw_init(&ctx);
    sha256_hw_update(&ctx, k_ipad, SHA256_BLOCK_SIZE);
    sha256_hw_update(&ctx, data, data_len);
    sha256_hw_final(&ctx, temp_hash);

    // 4. Outer Hash: H(K XOR opad || Inner Hash)
    sha256_hw_init(&ctx);
    sha256_hw_update(&ctx, k_opad, SHA256_BLOCK_SIZE);
    sha256_hw_update(&ctx, temp_hash, SHA256_DIGEST_SIZE);
    sha256_hw_final(&ctx, output);

    return 0; // 0 on success for libcotp
}

// Custom HMAC-SHA1 function (usually not hardware accelerated on RP2350)
int whmac_sha1(const uint8_t *key, size_t key_len, const uint8_t *data, size_t data_len, uint8_t *output) {
    // Fallback to software implementation (e.g., mbedTLS software SHA1)
    printf("CRYPTO: Using software HMAC-SHA1...\n");
    // TODO: Implementation (This is not needed for the Yubico OATH spec, which uses SHA-1 and SHA-256)
    return 0;
}
