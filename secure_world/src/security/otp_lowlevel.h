#ifndef _OTP_LOWLEVEL_H_
#define _OTP_LOWLEVEL_H_

#include <stdint.h>
#include <stdbool.h>
#include "hardware/regs/otp.h"
#include "hardware/structs/otp.h"
#include "pico/bootrom.h" // For rom_func_otp_access

//--------------------------------------------------------------------+
// RP2350 OTP Low-Level Definitions
//--------------------------------------------------------------------+

// Master Key is 256 bits (32 bytes or 8 words)
#define MASTER_KEY_SIZE_BYTES 32
#define MASTER_KEY_SIZE_WORDS 8

// We will use OTP Page 48 (index 48) for the Master Key
// OTP Page size is 128 bytes (32 words)
#define OTP_MASTER_KEY_PAGE_INDEX 48
#define OTP_MASTER_KEY_OFFSET (OTP_MASTER_KEY_PAGE_INDEX * 32 * 4) // 48 * 128 bytes

// Memory-mapped address for the Master Key
#define OTP_MASTER_KEY_ADDR ((volatile uint32_t *)(OTP_DATA_BASE + OTP_MASTER_KEY_OFFSET))

// Soft-lock value for read-only access (3 = inaccessible)
// We use 0b1111 (all bits set) to make it inaccessible from both Secure and Non-Secure worlds
// This is the strongest soft-lock.
#define OTP_SW_LOCK_INACCESSIBLE 0b1111

//--------------------------------------------------------------------+
// OTP Access ROM Function Wrapper
//--------------------------------------------------------------------+

// The rom_func_otp_access is a function pointer in the boot ROM.
// We use the pico SDK's definition for this.

/**
 * @brief Writes data to the OTP memory using the BootROM function.
 * 
 * @param data_in Pointer to the data to write.
 * @param len_bytes Length of the data in bytes.
 * @param cmd The OTP command structure.
 * @return 0 on success, non-zero on failure.
 */
static inline uint32_t otp_write_rom(const void *data_in, size_t len_bytes, otp_cmd_t cmd) {
    // The ROM function expects the command flags to be set correctly for writing.
    // It handles ECC and redundant copies automatically.
    return rom_func_otp_access(data_in, len_bytes, cmd);
}

#endif // _OTP_LOWLEVEL_H_
