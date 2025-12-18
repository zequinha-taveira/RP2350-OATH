#include "applet_manager.h"
#include "secure_functions.h"
#include "secure_gateway.h"
#include "security/hsm.h"
#include <arm_cmse.h> // Arm TrustZone for v8-M
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/**
 * @file secure_gateway_s.c
 * @brief Secure World entry points for Non-Secure calls.
 *
 * Implements the Non-Secure Callable (NSC) interface. All calls from the
 * Non-Secure World (NSW) must pass through this handler.
 */

/**
 * Single entry point for all Non-Secure World calls.
 *
 * @param func_id Function identifier (R0)
 * @param in_data Pointer to input data in NS memory (R1)
 * @param in_len Length of input data (R2)
 * @param out_data Pointer to output buffer in NS memory (R3)
 * @return int32_t length written or negative error code.
 */
int32_t __cmse_nonsecure_entry
secure_world_handler(secure_gateway_func_id_t func_id, uint8_t *in_data,
                     uint16_t in_len, uint8_t *out_data, uint16_t out_max_len) {

  // 1. Security Check: Validate that buffers are indeed in Non-Secure memory.
  // This prevents "Confused Deputy" attacks where NSW passes a Secure address.
  if (in_data && in_len > 0) {
    if (cmse_check_address_range(in_data, in_len, CMSE_NONSECURE | CMSE_AUIP) ==
        NULL) {
      printf("[GATEWAY] CRITICAL: Input buffer %p[%u] not in NS memory!\n",
             in_data, in_len);
      return -100; // Security Error
    }
  }

  if (out_data && out_max_len > 0) {
    if (cmse_check_address_range(out_data, out_max_len,
                                 CMSE_NONSECURE | CMSE_AUIP) == NULL) {
      printf("[GATEWAY] CRITICAL: Output buffer %p[%u] not in NS memory!\n",
             out_data, out_max_len);
      return -100;
    }
  }

  uint16_t out_len_val = 0;
  int32_t result = 0;

  switch (func_id) {
  case SG_INIT:
    applet_manager_init();
    result = 0;
    break;

  case SG_OATH_HANDLE_APDU:
    applet_manager_process_apdu(in_data, in_len, out_data, &out_len_val);
    result = (int32_t)out_len_val;
    break;

  case SG_HSM_GEN_KEY:
    if (in_len < 1 || !out_data || out_max_len < 1)
      return -1;
    out_data[0] = hsm_generate_key(in_data[0]);
    result = 1;
    break;

  case SG_HSM_GET_PUBKEY: {
    if (in_len < 1 || !out_data || out_max_len < 65)
      return -1;
    uint16_t pub_len = 0;
    out_data[0] = hsm_get_pubkey(in_data[0], out_data + 1, &pub_len);
    result = (int32_t)(pub_len + 1);
    break;
  }

  case SG_HSM_SIGN: {
    if (in_len < 33 || !out_data || out_max_len < 65)
      return -1;
    uint16_t sig_len = 0;
    out_data[0] = hsm_sign(in_data[0], in_data + 1, out_data + 1, &sig_len);
    result = (int32_t)(sig_len + 1);
    break;
  }

  default:
    printf("[GATEWAY] Unknown Function ID: 0x%02X\n", func_id);
    result = -2;
    break;
  }

  return result;
}
