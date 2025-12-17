#ifndef TIME_SYNC_H
#define TIME_SYNC_H

#include <stdint.h>
#include <stdbool.h>

/**
 * @brief Initialize time synchronization subsystem
 */
void time_sync_init(void);

/**
 * @brief Set the current Unix timestamp from host
 * @param timestamp Unix timestamp (seconds since epoch)
 */
void time_sync_set_timestamp(uint64_t timestamp);

/**
 * @brief Get current Unix timestamp
 * @return Current timestamp or 0 if not synced
 */
uint64_t time_sync_get_timestamp(void);

/**
 * @brief Check if time is synchronized
 * @return true if synced, false otherwise
 */
bool time_sync_is_synced(void);

/**
 * @brief Get time since last sync in seconds
 * @return Seconds since last sync
 */
uint32_t time_sync_get_age(void);

/**
 * @brief Handle time sync APDU command
 * @param data_in Input data containing timestamp
 * @param len_in Length of input data
 * @param data_out Output buffer for response
 * @param len_out Length of output data
 */
void time_sync_handle_apdu(uint8_t *data_in, uint16_t len_in, 
                          uint8_t *data_out, uint16_t *len_out);

#endif // TIME_SYNC_H