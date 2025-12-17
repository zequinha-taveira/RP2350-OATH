#ifndef FIDO2_STORAGE_H
#define FIDO2_STORAGE_H

#include <stdbool.h>
#include <stdint.h>


// FIDO2 Storage Configuration
// Using the 4th to last 4KB sector of Flash
#define FIDO2_FLASH_OFFSET (2 * 1024 * 1024 - 16384)

#define FIDO2_MAX_CREDENTIALS 10
#define FIDO2_ID_LEN 16
#define FIDO2_KEY_LEN 32

typedef struct {
  uint8_t credential_id[FIDO2_ID_LEN];
  uint8_t private_key[FIDO2_KEY_LEN];
  char user_id[32];
  char rp_id[64];
  uint32_t sign_count;
} fido2_credential_t;

typedef struct {
  uint32_t count;
  fido2_credential_t credentials[FIDO2_MAX_CREDENTIALS];
} fido2_storage_t;

/**
 * @brief Initializes FIDO2 storage.
 */
void fido2_storage_init(void);

/**
 * @brief Saves the current FIDO2 credentials to flash.
 */
void fido2_storage_save(void);

/**
 * @brief Returns a pointer to the FIDO2 storage.
 */
fido2_storage_t *fido2_storage_get_data(void);

#endif // FIDO2_STORAGE_H
