#ifndef OATH_STORAGE_H
#define OATH_STORAGE_H

#include <stdbool.h>
#include <stdint.h>

#define MAX_CREDENTIALS 16
#define OATH_MAX_NAME_LEN 64
#define OATH_MAX_SECRET_LEN 64 // Binary secret length (supports SHA512)

typedef enum { OATH_TYPE_HOTP = 0x10, OATH_TYPE_TOTP = 0x20 } oath_type_t;

typedef enum {
  OATH_ALGO_SHA1 = 0x01,
  OATH_ALGO_SHA256 = 0x02,
  OATH_ALGO_SHA512 = 0x03
} oath_algo_t;

typedef struct {
  char name[OATH_MAX_NAME_LEN];
  uint8_t secret[OATH_MAX_SECRET_LEN];
  uint8_t secret_len;
  oath_type_t type;
  oath_algo_t algorithm;
  uint8_t digits;
  uint32_t counter;       // For HOTP
  uint32_t period;        // For TOTP (default 30)
  uint8_t touch_required; // 0x01 if touch required
} oath_credential_t;

// Per-credential encryption structure
typedef struct {
  uint8_t iv[12];          // Initialization Vector (GCM)
  uint8_t ciphertext[256]; // Encrypted data (enough for oath_credential_t)
  uint8_t ciphertext_len;  // Length of ciphertext
  uint8_t tag[16];         // Authentication tag
} encrypted_credential_t;

// Persistence structure
typedef struct {
  uint32_t magic; // 0xDEADBEEF
  uint32_t version;
  uint8_t access_code_hash[32]; // SHA-256 hash of PIN
  uint8_t access_code_set;      // 0 = not set, 1 = set
  encrypted_credential_t encrypted_creds[MAX_CREDENTIALS];
  bool slot_used[MAX_CREDENTIALS];
  uint8_t master_key_salt[16]; // Salt for key derivation
} oath_persist_t;

// Initialize OATH storage
void oath_storage_init(void);

// Save a new credential
bool oath_storage_put(const char *name, const uint8_t *secret,
                      uint8_t secret_len, oath_type_t type, oath_algo_t algo,
                      uint8_t digits, uint32_t period, uint8_t touch_required);

// Delete a credential by name
bool oath_storage_delete(const char *name);

// Find a credential by name
bool oath_storage_get(const char *name, oath_credential_t *out_cred);

// List credentials (returns name at index)
const char *oath_storage_list(uint32_t index);

// Reset storage
void oath_storage_reset(void);

// Update counter (for HOTP)
bool oath_storage_update_counter(const char *name, uint32_t new_counter);

// Set Access Code (Password) - stores hash
bool oath_storage_set_password(const uint8_t *code, uint8_t len);

// Verify Access Code
bool oath_storage_verify_password(const uint8_t *code, uint8_t len);

// Check if any password is currently set
bool oath_storage_is_password_set(void);

// Helper functions for credential encryption (implemented in aes_gcm.c or
// oath_storage.c)
bool encrypt_credential(const uint8_t *key, const oath_credential_t *cred,
                        encrypted_credential_t *encrypted);
bool decrypt_credential(const uint8_t *key,
                        const encrypted_credential_t *encrypted,
                        oath_credential_t *cred);

#endif // OATH_STORAGE_H
