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
  uint32_t counter; // For HOTP
  uint32_t period;  // For TOTP (default 30)
} oath_credential_t;

// Initialize OATH storage
void oath_storage_init(void);

// Save a new credential
bool oath_storage_put(const char *name, const uint8_t *secret,
                      uint8_t secret_len, oath_type_t type, oath_algo_t algo,
                      uint8_t digits, uint32_t period);

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

#endif // OATH_STORAGE_H
