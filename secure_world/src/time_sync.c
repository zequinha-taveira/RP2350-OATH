#include "time_sync.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

// Global time state
static uint64_t g_base_timestamp = 0;
static uint64_t g_boot_time_us = 0;
static bool g_time_synced = false;

/**
 * @brief Initialize time synchronization subsystem
 */
void time_sync_init(void) {
    g_base_timestamp = 0;
    g_boot_time_us = time_us_64();
    g_time_synced = false;
    printf("Time Sync: Initialized (not synced)\n");
}

/**
 * @brief Set the current Unix timestamp from host
 * @param timestamp Unix timestamp (seconds since epoch)
 */
void time_sync_set_timestamp(uint64_t timestamp) {
    g_base_timestamp = timestamp;
    g_boot_time_us = time_us_64();
    g_time_synced = true;
    printf("Time Sync: Set timestamp to %llu\n", timestamp);
}

/**
 * @brief Get current Unix timestamp
 * @return Current timestamp or 0 if not synced
 */
uint64_t time_sync_get_timestamp(void) {
    if (!g_time_synced) {
        return 0;
    }
    
    uint64_t elapsed_us = time_us_64() - g_boot_time_us;
    uint64_t elapsed_sec = elapsed_us / 1000000ULL;
    return g_base_timestamp + elapsed_sec;
}

/**
 * @brief Check if time is synchronized
 * @return true if synced, false otherwise
 */
bool time_sync_is_synced(void) {
    return g_time_synced;
}

/**
 * @brief Get time since last sync in seconds
 * @return Seconds since last sync
 */
uint32_t time_sync_get_age(void) {
    if (!g_time_synced) {
        return 0xFFFFFFFF; // Max value if never synced
    }
    
    uint64_t elapsed_us = time_us_64() - g_boot_time_us;
    return (uint32_t)(elapsed_us / 1000000ULL);
}

/**
 * @brief Handle time sync APDU command
 * @param data_in Input data containing timestamp
 * @param len_in Length of input data
 * @param data_out Output buffer for response
 * @param len_out Length of output data
 */
void time_sync_handle_apdu(uint8_t *data_in, uint16_t len_in, 
                          uint8_t *data_out, uint16_t *len_out) {
    // Expected format: 8 bytes Unix timestamp (big-endian)
    if (len_in < 8) {
        *len_out = 0;
        return;
    }
    
    uint64_t timestamp = 0;
    for (int i = 0; i < 8; i++) {
        timestamp = (timestamp << 8) | data_in[i];
    }
    
    time_sync_set_timestamp(timestamp);
    
    // Response: 1 byte status (0x01 = success)
    data_out[0] = 0x01;
    *len_out = 1;
}