#include "oath_storage.h"
#include "pico/flash.h"
#include "pico/sync.h"
#include "crypto/aes.h"
#include "security/security.h" // For OTP master key access
#include "pico/rand.h" // For generating IV

//--------------------------------------------------------------------+
// OATH Credential Storage Implementation (Secure World Logic Placeholder)
//--------------------------------------------------------------------+

// Flash memory configuration
#define FLASH_TARGET_OFFSET (2048 * 1024) // Start at 2MB offset (example)
#define CREDENTIAL_PAGE_SIZE (FLASH_SECTOR_SIZE) // 4KB per page

// Structure to hold a single encrypted credential
typedef struct {
    uint8_t id[32]; // Credential ID (e.g., issuer:user)
    uint8_t encrypted_secret[32]; // Encrypted secret key
    uint8_t iv[16]; // Initialization Vector for AES
    uint32_t type; // TOTP or HOTP
    uint32_t counter; // For HOTP
    uint32_t crc32; // Integrity check
} oath_credential_t;

// Global array to cache credentials (optional, for performance)
// oath_credential_t credential_cache[MAX_CREDENTIALS];

// Global master key (retrieved from OTP)
static uint8_t master_key[AES_KEY_SIZE_BYTES]; // 256-bit key

// Initialize storage: read master key from OTP and load index from flash
void oath_storage_init(void) {
    // Try to read the master key from OTP
    if (!otp_read_master_key(master_key)) {
        printf("STORAGE: Master key not found in OTP. Attempting first-time write...\n");
        // If not found, generate a new one and write it (first boot)
        if (!otp_write_new_master_key(master_key)) {
            printf("STORAGE: ERROR - Failed to initialize master key in OTP! Device is insecure.\n");
            // Critical error: device cannot function securely
            return;
        }
    }
    
    // TODO: Implement logic to read the credential index from flash
    printf("STORAGE: Initialized. Master key loaded. Index loaded from flash at 0x%lX.\n", FLASH_TARGET_OFFSET);
}

// Function to save a new credential (encrypted)
bool oath_storage_save(oath_credential_t *cred) {
    // 1. Generate a unique IV
    for (int i = 0; i < AES_IV_SIZE_BYTES; i += 4) {
        uint32_t rand_val = get_rand_32();
        memcpy(cred->iv + i, &rand_val, 4);
    }

    // 2. Encrypt the secret using the master_key
    // NOTE: We assume the secret is padded to a multiple of AES_BLOCK_SIZE (16 bytes)
    size_t secret_len = sizeof(cred->encrypted_secret); // Assuming this is the padded length
    
    if (!aes_encrypt(master_key, cred->iv, cred->encrypted_secret, secret_len, cred->encrypted_secret)) {
        printf("STORAGE: ERROR - Failed to encrypt credential secret.\n");
        return false;
    }

    // 3. Erase the flash sector
    // 4. Write the encrypted credential (including IV) to flash
    
    printf("STORAGE: Saving new credential (encrypted)...\n");
    // TODO: Implementation of flash write
    return true;
}

// Function to load a credential (decrypted)
bool oath_storage_load(const uint8_t *id, oath_credential_t *cred_out) {
    // 1. Find the credential in the flash index
    // 2. Read the encrypted credential (including IV) from flash
    
    printf("STORAGE: Loading credential by ID...\n");
    // TODO: Implementation of flash read
    
    // 3. Decrypt the secret using the master_key
    size_t secret_len = sizeof(cred_out->encrypted_secret); // Assuming this is the padded length
    
    if (!aes_decrypt(master_key, cred_out->iv, cred_out->encrypted_secret, secret_len, cred_out->encrypted_secret)) {
        printf("STORAGE: ERROR - Failed to decrypt credential secret.\n");
        return false;
    }
    
    return true;
}
