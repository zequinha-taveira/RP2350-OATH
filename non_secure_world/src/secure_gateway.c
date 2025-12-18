#include "secure_gateway.h"
#include "secure_functions.h"
#include "secure_gateway.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// Secure World Handler from secure_functions.h
extern int32_t secure_world_handler(secure_gateway_func_id_t func_id,
                                    uint8_t *in_data, uint16_t in_len,
                                    uint8_t *out_data, uint16_t out_max_len);

//--------------------------------------------------------------------+
// Secure Gateway Implementation (Non-Secure World Side)
//--------------------------------------------------------------------+

void secure_gateway_init(void) {
  printf("NS-Gateway: Initializing Secure World...\n");
  secure_world_handler(SG_INIT, NULL, 0, NULL, 0);
  printf("NS-Gateway: Secure World initialization complete.\n");
}

bool secure_gateway_oath_handle_apdu(uint8_t *apdu_in, uint16_t len_in,
                                     uint8_t *apdu_out, uint16_t *len_out) {
  // Max APDU response size is usually 256 + SW
  int32_t result = secure_world_handler(SG_OATH_HANDLE_APDU, apdu_in, len_in,
                                        apdu_out, 1024);
  if (result < 0)
    return false;
  *len_out = (uint16_t)result;
  return true;
}

bool secure_gateway_hsm_gen_key(uint8_t slot, uint8_t *status) {
  uint8_t in_data = slot;
  uint8_t out_data[1];
  int32_t result =
      secure_world_handler(SG_HSM_GEN_KEY, &in_data, 1, out_data, 1);
  if (result < 0)
    return false;
  *status = out_data[0];
  return true;
}

bool secure_gateway_hsm_get_pubkey(uint8_t slot, uint8_t *pubkey,
                                   uint16_t *pubkey_len) {
  uint8_t in_data = slot;
  uint8_t out_data[65]; // 1 byte status + 64 bytes pubkey
  int32_t result =
      secure_world_handler(SG_HSM_GET_PUBKEY, &in_data, 1, out_data, 65);
  if (result < 1)
    return false;
  if (out_data[0] != 0)
    return false; // HSM_STATUS_OK = 0
  *pubkey_len = (uint16_t)(result - 1);
  memcpy(pubkey, out_data + 1, *pubkey_len);
  return true;
}

bool secure_gateway_hsm_sign(uint8_t slot, const uint8_t *hash, uint8_t *sig,
                             uint16_t *sig_len) {
  uint8_t in_data[33];
  in_data[0] = slot;
  memcpy(in_data + 1, hash, 32);

  uint8_t out_data[65]; // 1 byte status + 64 bytes sig
  int32_t result = secure_world_handler(SG_HSM_SIGN, in_data, 33, out_data, 65);
  if (result < 1)
    return false;
  if (out_data[0] != 0)
    return false;
  *sig_len = (uint16_t)(result - 1);
  memcpy(sig, out_data + 1, *sig_len);
  return true;
}
