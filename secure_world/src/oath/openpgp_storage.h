#ifndef OPENPGP_STORAGE_H
#define OPENPGP_STORAGE_H

#include <stdbool.h>
#include <stdint.h>


// OpenPGP Storage Configuration
// Using the 3rd to last 4KB sector of Flash
#define OPENPGP_FLASH_OFFSET (2 * 1024 * 1024 - 12288)

#define OPENPGP_MAX_NAME_LEN 32
#define OPENPGP_MAX_SERIAL_LEN 6

typedef struct {
  uint8_t serial[OPENPGP_MAX_SERIAL_LEN];
  char name[OPENPGP_MAX_NAME_LEN];
  uint8_t lang[2];
  uint8_t sex;
  uint8_t pin_retry_counter[3]; // PW1, PW1_83, PW3
  // In a real app, we'd store hashed/encrypted PINs here
} openpgp_data_t;

/**
 * @brief Initializes OpenPGP storage.
 */
void openpgp_storage_init(void);

/**
 * @brief Saves the current OpenPGP data to flash.
 */
void openpgp_storage_save(void);

/**
 * @brief Returns a pointer to the current OpenPGP data.
 */
openpgp_data_t *openpgp_storage_get_data(void);

#endif // OPENPGP_STORAGE_H
