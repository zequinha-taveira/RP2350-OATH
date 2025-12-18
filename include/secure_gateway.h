#ifndef _SECURE_GATEWAY_H_
#define _SECURE_GATEWAY_H_

#include <stdbool.h>
#include <stdint.h>

//--------------------------------------------------------------------+
// Secure Gateway Interface (Non-Secure World Side)
//--------------------------------------------------------------------+

// Secure Gateway function IDs (used to dispatch calls in the Secure World)
typedef enum {
  SG_INIT = 0x01,
  SG_OATH_HANDLE_APDU = 0x02,
  SG_GET_TIME = 0x03,
  SG_HSM_GEN_KEY = 0x10,
  SG_HSM_GET_PUBKEY = 0x11,
  SG_HSM_SIGN = 0x12,
} secure_gateway_func_id_t;

// Secure Gateway error codes
#define SG_SUCCESS 0
#define SG_ERR_SECURITY -100
#define SG_ERR_INVALID_PARAM -1
#define SG_ERR_UNKNOWN_FUNC -2
#define SG_ERR_BUFFER_TOO_SMALL -3
#define SG_ERR_TOUCH_REQUIRED -4 // Special case for non-blocking touch

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
bool secure_gateway_oath_handle_apdu(uint8_t *apdu_in, uint16_t len_in,
                                     uint8_t *apdu_out, uint16_t *len_out);

/**
 * @brief Calls the Secure World to generate an HSM key.
 */
bool secure_gateway_hsm_gen_key(uint8_t slot, uint8_t *status);

/**
 * @brief Calls the Secure World to get an HSM public key.
 */
bool secure_gateway_hsm_get_pubkey(uint8_t slot, uint8_t *pubkey,
                                   uint16_t *pubkey_len);

/**
 * @brief Calls the Secure World to sign data with an HSM key.
 */
bool secure_gateway_hsm_sign(uint8_t slot, const uint8_t *hash, uint8_t *sig,
                             uint16_t *sig_len);

#endif // _SECURE_GATEWAY_H_
