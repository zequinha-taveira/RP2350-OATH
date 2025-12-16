#ifndef _SECURITY_H_
#define _SECURITY_H_

#include <stdbool.h>
#include <stdint.h>

//--------------------------------------------------------------------+
// Security API
//--------------------------------------------------------------------+

/**
 * @brief Initializes all hardware security features.
 * 
 * This includes enabling glitch detectors (if configured in OTP),
 * initializing the hardware SHA-256 accelerator, and checking Secure Boot status.
 */
void security_init(void);

/**
 * @brief Reads the 256-bit master key from the soft-locked OTP region.
 * 
 * This function must use the low-level hardware access to the OTP memory.
 * 
 * @param key_out Pointer to a 32-byte buffer to store the key.
 * @return true if the key was successfully read, false otherwise.
 */
bool otp_read_master_key(uint8_t *key_out);

/**
 * @brief Generates a new 256-bit master key and writes it to the OTP.
 * 
 * This function is only called on the very first boot. It uses the TRNG.
 * 
 * @param key_out Pointer to a 32-byte buffer to store the generated key.
 * @return true if the key was successfully written and soft-locked, false otherwise.
 */
bool otp_write_new_master_key(uint8_t *key_out);

/**
 * @brief Checks the status of the Secure Boot chain.
 * 
 * @return true if the firmware was signed and verified by the BootROM, false otherwise.
 */
bool secure_boot_check(void);

#endif // _SECURITY_H_
