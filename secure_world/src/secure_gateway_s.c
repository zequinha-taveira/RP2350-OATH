#include "../../include/secure_functions.h"
#include "../../include/secure_gateway.h"
#include "applet_manager.h"
#include "security/hsm.h"
#include <arm_cmse.h> // Arm TrustZone for v8-M
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h> // Added missing standard header
#include <string.h>


#include "oath/fido2_applet.h"
#include "oath/oath_storage.h"

/**
 * @file secure_gateway_s.c
 * @brief Secure World entry points for Non-Secure calls.
 */

// Sensitive data scrubbing helper
static void scrub_buffer(void *buf, size_t len) {
  if (buf) {
    volatile uint8_t *p = (volatile uint8_t *)buf;
    while (len--)
      *p++ = 0;
  }
}

/**
 * Single entry point for all Non-Secure World calls.
 */
__attribute__((cmse_nonsecure_entry)) int32_t
secure_world_handler(secure_gateway_func_id_t func_id, uint8_t *in_data,
                     uint16_t in_len, uint8_t *out_data, uint16_t out_max_len) {

  // 1. Security Check: Validate that buffers are indeed in Non-Secure memory.
  if (in_data != NULL && in_len > 0) {
    if (cmse_check_address_range(in_data, in_len, CMSE_NONSECURE | CMSE_AUIP) ==
        NULL) {
      return SG_ERR_SECURITY;
    }
  }

  if (out_data != NULL && out_max_len > 0) {
    if (cmse_check_address_range(out_data, out_max_len,
                                 CMSE_NONSECURE | CMSE_AUIP) == NULL) {
      return SG_ERR_SECURITY;
    }
    // Zero out output buffer initially for security
    memset(out_data, 0, out_max_len);
  }

  uint16_t out_len_val = 0;
  int32_t result = 0;

  switch (func_id) {
  case SG_INIT:
    applet_manager_init();
    result = SG_SUCCESS;
    break;

  case SG_OATH_HANDLE_APDU:
    if (!in_data || !out_data) {
      result = SG_ERR_INVALID_PARAM;
    } else {
      applet_manager_process_apdu(in_data, in_len, out_data, &out_len_val);
      result = (int32_t)out_len_val;
    }
    break;

  case SG_GET_CONFIG:
    if (!out_data || out_max_len < 4) {
      result = SG_ERR_INVALID_PARAM;
    } else {
      // Return VID (1050) and PID (0407)
      out_data[0] = 0x50; // 1050 LE: 0x1050 -> 50 10? No, 1050 = 0x041A. No,
                          // Yubico VID is 0x1050.
      out_data[1] = 0x10;
      out_data[2] = 0x07; // 0x0407
      out_data[3] = 0x04;
      result = 4;
    }
    break;

  case SG_HSM_GEN_KEY:
    if (in_len < 1 || !out_data || out_max_len < 1) {
      result = SG_ERR_INVALID_PARAM;
    } else {
      out_data[0] = hsm_generate_key(in_data[0]);
      result = 1;
    }
    break;

  case SG_HSM_GET_PUBKEY: {
    if (in_len < 1 || !out_data || out_max_len < 65) {
      result = SG_ERR_INVALID_PARAM;
    } else {
      uint16_t pub_len = 0;
      out_data[0] = hsm_get_pubkey(in_data[0], out_data + 1, &pub_len);
      result = (int32_t)(pub_len + 1);
    }
    break;
  }

  case SG_HSM_SIGN: {
    if (in_len < 33 || !out_data || out_max_len < 65) {
      result = SG_ERR_INVALID_PARAM;
    } else {
      uint16_t sig_len = 0;
      out_data[0] = hsm_sign(in_data[0], in_data + 1, out_data + 1, &sig_len);
      result = (int32_t)(sig_len + 1);
    }
    scrub_buffer(in_data,
                 in_len); // Scrub sensitive input after use (if applicable)
    break;
  }

  case SG_FIDO2_HANDLE_MSG: {
    if (in_data == NULL || out_data == NULL) {
      result = SG_ERR_INVALID_PARAM;
    } else {
      uint16_t out_len = 0;
      fido2_applet_handle_msg(in_data, in_len, out_data, &out_len);
      result = (int32_t)out_len;
    }
    break;
  }

  case SG_OATH_BACKUP: {
    if (out_data == NULL || out_max_len < sizeof(oath_persist_t)) {
      result = SG_ERR_INVALID_PARAM;
    } else {
      uint16_t out_len = out_max_len;
      if (oath_storage_export(out_data, &out_len)) {
        result = (int32_t)out_len;
      } else {
        result = SG_ERR_SECURITY;
      }
    }
    break;
  }

  case SG_OATH_RESTORE: {
    if (in_data == NULL || in_len != sizeof(oath_persist_t)) {
      result = SG_ERR_INVALID_PARAM;
    } else {
      if (oath_storage_import(in_data, in_len)) {
        result = SG_SUCCESS;
      } else {
        result = SG_ERR_SECURITY;
      }
    }
    break;
  }

  default:
    result = SG_ERR_UNKNOWN_FUNC;
    break;
  }

  return result;
}
