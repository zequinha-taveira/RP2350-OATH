#include "oath_protocol.h"    // Secure World OATH logic
#include "secure_functions.h" // Ensure definition of __cmse_nonsecure_entry
#include "secure_gateway.h"   // Shared header for function IDs
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
    return 0; // Success

  case SG_OATH_HANDLE_APDU:
    // Call the core OATH logic
    // We pass &out_len_val to capture the output length
    // We assume the caller allocated enough space in out_data (e.g. 256 bytes)
    oath_handle_apdu(in_data, in_len, out_data, &out_len_val);
    return (int32_t)out_len_val;

  case SG_GET_TIME:
    printf("Secure World: SG_GET_TIME called (TODO).\n");
    return -1; // Not implemented

  default:
    printf("Secure World: Unknown function ID 0x%02X.\n", func_id);
    return -2; // Unknown function
  }
}
