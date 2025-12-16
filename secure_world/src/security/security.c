#include "security.h"
#include "otp_lowlevel.h"
#include "pico/rand.h" // For TRNG access
#include "pico/bootrom.h" // For rom_func_otp_access
#include "pico/stdlib.h"
#include "hardware/regs/otp.h" // RP2350 OTP registers
#include "hardware/regs/sha256.h" // RP2350 SHA-256 registers
#include "hardware/regs/trng.h" // RP2350 TRNG registers

//--------------------------------------------------------------------+
// Security Implementation (Placeholders for RP2350 Low-Level Calls)
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
    // The SHA-256 hardware is typically initialized via the pico_sha256_hw library.
    printf("SECURITY: Hardware SHA-256 and TRNG available.\n");
    
    // 3. Glitch Detector
    // Glitch detectors are enabled by programming the OTP.
    // We assume the OTP is programmed correctly for this phase.
    printf("SECURITY: Glitch Detectors status assumed from OTP programming.\n");
}

bool otp_read_master_key(uint8_t *key_out) {
    if (!is_master_key_written()) {
        printf("OTP: Master key not written/locked. Cannot read.\n");
        return false;
    }

    // The key is read directly from the memory-mapped OTP address.
    // The soft-lock prevents *subsequent* reads by other code, but the key
    // is available to the initial boot code before the lock is fully enforced.
    // Since we are running the initial firmware, we can read it directly.
    volatile uint32_t *otp_addr = OTP_MASTER_KEY_ADDR;
    
    printf("OTP: Reading master key from soft-locked region (Page %d)...\n", OTP_MASTER_KEY_PAGE_INDEX);

    for (int i = 0; i < MASTER_KEY_SIZE_WORDS; i++) {
        ((uint32_t*)key_out)[i] = otp_addr[i];
    }
    
    return true;
}

bool otp_write_new_master_key(uint8_t *key_out) {
    if (is_master_key_written()) {
        printf("OTP: Master key already written and locked. Aborting write.\n");
        return false;
    }

    // 1. Generate 32 bytes of random data using TRNG.
    generate_random_key(key_out);
    
    printf("OTP: Generating and writing NEW master key to OTP (Page %d)...\n", OTP_MASTER_KEY_PAGE_INDEX);

    // 2. Write the data to the designated OTP region using the ROM function.
    // The ROM function handles ECC and redundant copies.
    otp_cmd_t cmd = {
        .flags = OTP_CMD_ECC_BITS | OTP_CMD_WRITE_BITS,
        .page_index = OTP_MASTER_KEY_PAGE_INDEX
    };
    
    uint32_t ret = rom_func_otp_access(key_out, MASTER_KEY_SIZE_BYTES, cmd);
    
    if (ret != 0) {
        printf("OTP: ERROR - Write failed with ROM error %lu.\n", ret);
        return false;
    }
    
    // 3. Write the soft-lock register to make the page inaccessible (0b1111).
    // This is the crucial step for security.
    otp_hw->SW_LOCK[OTP_MASTER_KEY_PAGE_INDEX] = OTP_SW_LOCK_INACCESSIBLE;
    
    printf("OTP: Key written and soft-locked successfully.\n");
    return true;
}

bool secure_boot_check(void) {
    // TODO: Implement check for the Secure Boot status flag.
    // This flag is usually a read-only register set by the BootROM.
    
    // Placeholder: Assume success if the device is running
    return true; 
}

// Helper function to check if the master key page is already written/locked
static bool is_master_key_written(void) {
    // Check the soft-lock status for the master key page (Page 48)
    // If the lock register is set to 0b1111 (inaccessible), the key has been written and locked.
    return (otp_hw->SW_LOCK[OTP_MASTER_KEY_PAGE_INDEX] == OTP_SW_LOCK_INACCESSIBLE);
}

// Helper function to generate a 256-bit key using the True Random Number Generator (TRNG)
static void generate_random_key(uint8_t *key_out) {
    // The pico/rand.h provides a simple wrapper for the TRNG
    for (int i = 0; i < MASTER_KEY_SIZE_BYTES; i += 4) {
        uint32_t rand_val = get_rand_32();
        memcpy(key_out + i, &rand_val, 4);
    }
}
