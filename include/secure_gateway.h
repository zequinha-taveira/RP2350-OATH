#ifndef _SECURE_GATEWAY_H_
#define _SECURE_GATEWAY_H_

#include <stdint.h>
#include <stdbool.h>

//--------------------------------------------------------------------+
// Secure Gateway Interface (Non-Secure World Side)
//--------------------------------------------------------------------+

// Secure Gateway function IDs (used to dispatch calls in the Secure World)
typedef enum {
    SG_INIT = 0x01,
    SG_OATH_HANDLE_APDU = 0x02,
    SG_GET_TIME = 0x03,
    // Add more functions as needed
} secure_gateway_func_id_t;

/**
 * @brief Initializes the Secure World environment.
 * 
 * In a TF-M environment, this would be part of the boot process.
 * For this skeleton, it's a placeholder for the NS side to signal readiness.
 */
void secure_gateway_init(void);

/**
 * @brief Calls the Secure World to handle an OATH APDU command.
 * 
 * This function is the Non-Secure side of the Secure Gateway call.
 * It packages the APDU data and calls the Secure World entry point.
 * 
 * @param apdu_in Pointer to the incoming APDU command buffer.
 * @param len_in Length of the incoming APDU command.
 * @param apdu_out Pointer to the outgoing APDU response buffer.
 * @param len_out Pointer to the length of the outgoing APDU response.
 * @return true on success, false on failure.
 */
bool secure_gateway_oath_handle_apdu(uint8_t *apdu_in, uint16_t len_in, uint8_t *apdu_out, uint16_t *len_out);

#endif // _SECURE_GATEWAY_H_
