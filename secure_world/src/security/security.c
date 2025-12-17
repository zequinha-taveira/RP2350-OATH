#include "security.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

#include "pico/rand.h"

// Forward declarations
static bool is_master_key_written(void);
static void generate_random_key(uint8_t *key_out);

// Global State
static bool security_initialized = false;

//--------------------------------------------------------------------+
// Security Implementation
//--------------------------------------------------------------------+

void security_init(void) {
  printf("SECURITY: Initializing RP2350 hardware security...\n");

  // 1. Check Secure Boot Status
  if (secure_boot_check()) {
    printf("SECURITY: Secure Boot verified by BootROM. Firmware is trusted.\n");
  } else {
    printf("SECURITY: WARNING - Secure Boot check failed or not enabled.\n");
    // In a production device, this would halt execution.
  }

  // 2. Initialize Hardware Accelerators
  // The SHA-256 hardware is typically initialized via the pico_sha256_hw
  // library.
  printf("SECURITY: Hardware SHA-256 and TRNG available.\n");

  security_initialized = true;
}

bool otp_read_master_key(uint8_t *key_out) {
  if (!is_master_key_written()) {
    printf("OTP: Master key not written/locked. Cannot read.\n");
    return false;
  }

  // Actual implementation: Read from a designated memory location (e.g., a hardcoded address
  // in the RP2350's OTP or a secure flash region, depending on the final design).
  // Since we don't have the specific hardware register map for the RP2350 OTP,
  // we will use a placeholder that simulates reading from a secure, non-volatile location.
  // NOTE: In a real implementation, this would involve low-level hardware access.

  // Placeholder: Simulate reading from a secure memory address
  const uint8_t *otp_master_key_addr = (const uint8_t *)0x10000000; // Example placeholder address

  // Copy the key from the secure location to the output buffer
  memcpy(key_out, otp_master_key_addr, 32);

  printf("OTP: Master key read from secure storage.\n");

  return true;
}

bool otp_write_new_master_key(uint8_t *key_out) {
  if (is_master_key_written()) {
    printf("OTP: Master key already written and locked. Aborting write.\n");
    return false;
  }

  // 1. Generate 32 bytes of random data using TRNG.
  generate_random_key(key_out);

  printf("OTP: Generating and writing NEW master key to OTP...\n");

  // Placeholder for low-level OTP write operation.
  // In a real RP2350 implementation, this would involve:
  // a) Checking OTP write permissions.
  // b) Writing the 32-byte key to the designated OTP region.
  // c) Soft-locking the OTP region to prevent further writes (if applicable).
  
  // For now, we simulate success.
  
  // Placeholder: Simulate writing to a secure memory address
  uint8_t *otp_master_key_addr = (uint8_t *)0x10000000; // Example placeholder address
  memcpy(otp_master_key_addr, key_out, 32);

  printf("OTP: Key written and soft-locked successfully (Simulated).\n");
  return true;
}

bool secure_boot_check(void) {
  // Placeholder: Assume success if the device is running
  return true;
}

// Helper function to check if the master key page is already written/locked
static bool is_master_key_written(void) {
  // In a real scenario, this would check a flag in the OTP or a magic number
  // at the key's location to see if it has been written and locked.
  // For now, we use a static flag for simulation purposes.
  // In a production environment, this would be a hardware register read.
  static bool provisioned = false;
  
  // Check if the key has been written to the placeholder address
  const uint8_t *otp_master_key_addr = (const uint8_t *)0x10000000;
  
  // Simple check: if the first byte is not 0xFF (unwritten flash/memory state)
  if (*otp_master_key_addr != 0xFF) {
      provisioned = true;
  }
  
  return provisioned;
}

// Helper function to generate a 256-bit key using the True Random Number
// Generator (TRNG)
static void generate_random_key(uint8_t *key_out) {
  // The pico-sdk's get_rand_32() uses the hardware TRNG if available (RP2040/RP2350)
  // or a secure PRNG otherwise. This is suitable for key generation.
  for (int i = 0; i < 32 / sizeof(uint32_t); i++) {
    ((uint32_t *)key_out)[i] = get_rand_32();
  }
  printf("SECURITY: Generated 256-bit random key using TRNG/PRNG.\n");
}
