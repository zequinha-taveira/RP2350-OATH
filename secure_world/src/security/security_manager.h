#ifndef SECURITY_MANAGER_H
#define SECURITY_MANAGER_H

#include <stdint.h>

/**
 * @file security_manager.h
 * @brief Centralized security and memory layout configuration for RP2350-OATH.
 */

// Flash Memory Layout (2MB Total)
// We designate the last 128KB for secure persistent data
#define FLASH_SIZE_TOTAL (2 * 1024 * 1024)

// Simulated OTP region (for Master Key)
#define SIMULATED_OTP_BASE_ADDR (FLASH_SIZE_TOTAL - 4096) // Last sector (4KB)
#define OTP_MASTER_KEY_OFFSET (0x00)
#define OTP_LOCK_FLAG_OFFSET (0x40)

// OATH Storage region
// Located 2nd to last sector
#define OATH_FLASH_SECTOR_OFFSET (FLASH_SIZE_TOTAL - 8192)

// HSM Storage region
// Located 3rd to last sector
#define HSM_FLASH_OFFSET (FLASH_SIZE_TOTAL - 12288)

// Security Constraints
#define MAX_PIN_LENGTH 64
#define ACCESS_CODE_HASH_SIZE 32

#endif // SECURITY_MANAGER_H
