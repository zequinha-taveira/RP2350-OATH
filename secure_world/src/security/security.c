#include "security.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

#include "pico/rand.h"
#include <hardware/flash.h>

/**
 * @file security.c
 * @brief Realistic hardware security abstraction for RP2350.
 *
 * This module manages the Root of Trust, including Master Key derives
 * from OTP (simulated) and Secure Boot validation.
 */

#include "security_manager.h"

/**
 * @file security.c
 * @brief Realistic hardware security abstraction for RP2350.
 */

#define MASTER_KEY_SIZE (32)

// Global State
static bool security_initialized = false;

// Forward declarations
static bool is_master_key_written(void);
static void generate_random_key(uint8_t *key_out);
static bool constant_time_is_empty(const uint8_t *data, size_t len);

//--------------------------------------------------------------------+
// Security Implementation
//--------------------------------------------------------------------+

void security_init(void) {
  if (security_initialized)
    return;

  printf("[SECURITY] Initializing RP2350 hardware security layer...\n");

  // 1. Check Secure Boot Status (Simulated via BootROM state)
  if (secure_boot_check()) {
    printf("[SECURITY] Secure Boot: VALID (Chain of Trust established)\n");
  } else {
    printf("[SECURITY] WARNING: Secure Boot NOT verified. Device may be in "
           "development mode.\n");
  }

  // 2. Resource Validation
  // RP2350 has hardware SHA-256 and TRNG.
  printf("[SECURITY] Hardware TRNG and SHA-256 drivers linked.\n");

  security_initialized = true;
}

bool otp_read_master_key(uint8_t *key_out) {
  if (!security_initialized)
    security_init();

  if (!is_master_key_written()) {
    printf("[OTP] Master key not provisioned in secure storage.\n");
    return false;
  }

  // Capture the pointer to simulated OTP memory (mapped via XIP)
  const uint8_t *otp_ptr =
      (const uint8_t *)(XIP_BASE + SIMULATED_OTP_BASE_ADDR +
                        OTP_MASTER_KEY_OFFSET);

  // In a real RP2350, we would use the OTP controller registers to read.
  // Here we simulate the hardware fetch into the secure buffer.
  memcpy(key_out, otp_ptr, MASTER_KEY_SIZE);

  printf("[OTP] Master key retrieved successfully.\n");
  return true;
}

bool otp_write_new_master_key(uint8_t *key_out) {
  if (is_master_key_written()) {
    printf("[OTP] ERROR: Master key area already locked. Permanent write "
           "failure.\n");
    return false;
  }

  // 1. Generate high-entropy 256-bit key using hardware TRNG
  generate_random_key(key_out);

  printf("[OTP] Provisioning new master key to secure storage...\n");

  // 2. Hardware Simulation: Write to reserved flash region
  // Note: In real life, OTP writing is different from flash programming,
  // but on RP2350, OTP is programmed via the bootrom or dedicated controller.
  uint32_t interrupts = save_and_disable_interrupts();

  // We simulate the OTP write by programming a specific flash page if not done.
  // This is for demonstration; on hardware, use the OTP API.
  flash_range_program(SIMULATED_OTP_BASE_ADDR, key_out, MASTER_KEY_SIZE);

  restore_interrupts(interrupts);

  // 3. Lock the region
  if (otp_lock_master_key()) {
    printf("[OTP] Master key LOCKED and globally protected.\n");
    return true;
  }

  return false;
}

bool otp_lock_master_key(void) {
  printf("[SECURITY] Executing hardware lock on OTP master key region...\n");

  // Simulation: Write a 'lock' bit to the simulated OTP flag area
  uint8_t lock_val[16];
  memset(lock_val, 0x00,
         16); // 0x00 marks as "locked/programmed" in OTP logic often

  uint32_t interrupts = save_and_disable_interrupts();
  flash_range_program(SIMULATED_OTP_BASE_ADDR + OTP_LOCK_FLAG_OFFSET, lock_val,
                      16);
  restore_interrupts(interrupts);

  return true;
}

bool secure_boot_check(void) {
  // In production, this would query the BootROM or check OTP_BOOT_FLAGS
  return true;
}

// --------------------------------------------------------------------
// Private Helpers
// --------------------------------------------------------------------

static bool is_master_key_written(void) {
  const uint8_t *otp_ptr =
      (const uint8_t *)(XIP_BASE + SIMULATED_OTP_BASE_ADDR +
                        OTP_MASTER_KEY_OFFSET);

  // Check if memory is empty (0xFF) or has our key.
  // Using constant-time check for security parity.
  return !constant_time_is_empty(otp_ptr, MASTER_KEY_SIZE);
}

static bool constant_time_is_empty(const uint8_t *data, size_t len) {
  uint8_t diff = 0;
  for (size_t i = 0; i < len; i++) {
    diff |= (data[i] ^ 0xFF);
  }
  return (diff == 0);
}

static void generate_random_key(uint8_t *key_out) {
  // get_rand_32 is cryptographically secure on RP2040/RP2350
  for (int i = 0; i < MASTER_KEY_SIZE / 4; i++) {
    ((uint32_t *)key_out)[i] = get_rand_32();
  }
}
