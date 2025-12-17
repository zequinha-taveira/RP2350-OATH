#include "applet_manager.h"   // Multi-applet manager
#include "secure_functions.h" // Ensure definition of __cmse_nonsecure_entry
#include "secure_gateway.h"   // Shared header for function IDs
#include "security/hsm.h"     // HSM logic
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

//--------------------------------------------------------------------+
// Secure Gateway Implementation (Secure World Side)
//--------------------------------------------------------------------+

// Secure World Handler - The single entry point for Non-Secure calls
// Arguments must fit in R0-R3.
// func_id: R0
// in_data: R1
// in_len:  R2
// out_data: R3
// Return: int32_t (Positive = Length written, Negative = Error)

int32_t __cmse_nonsecure_entry
secure_world_handler(secure_gateway_func_id_t func_id, uint8_t *in_data,
                     uint16_t in_len, uint8_t *out_data) {
  printf("Secure World: Received call 0x%02X.\n", func_id);

  uint16_t out_len_val = 0;

  switch (func_id) {
  case SG_INIT:
    printf("Secure World: SG_INIT called.\n");
    applet_manager_init();
    return 0; // Success

  case SG_OATH_HANDLE_APDU:
    // Call the Applet Manager to route the APDU
    applet_manager_process_apdu(in_data, in_len, out_data, &out_len_val);
    return (int32_t)out_len_val;

  case SG_GET_TIME:
    printf("Secure World: SG_GET_TIME called (TODO).\n");
    return -1; // Not implemented

  case SG_HSM_GEN_KEY:
    // in_data[0] is the slot index
    if (in_len < 1)
      return -3;
    uint8_t gen_status = hsm_generate_key(in_data[0]);
    out_data[0] = gen_status;
    return 1;

  case SG_HSM_GET_PUBKEY:
    // in_data[0] is the slot index
    if (in_len < 1)
      return -3;
    uint16_t pub_len = 0;
    uint8_t pub_status = hsm_get_pubkey(in_data[0], out_data + 1, &pub_len);
    out_data[0] = pub_status;
    return (int32_t)(pub_len + 1);

  case SG_HSM_SIGN:
    // in_data[0] is the slot index
    // in_data[1...32] is the hash
    if (in_len < 33)
      return -3;
    uint16_t sig_len = 0;
    uint8_t sig_status =
        hsm_sign(in_data[0], in_data + 1, out_data + 1, &sig_len);
    out_data[0] = sig_status;
    return (int32_t)(sig_len + 1);

  default:
    printf("Secure World: Unknown function ID 0x%02X.\n", func_id);
    return -2; // Unknown function
  }
}
