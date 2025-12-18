#include "fido2_applet.h"
#include "apdu_protocol.h"
#include "fido2_storage.h"
#include "iso7816_4.h"
#include <pico/rand.h>
#include <stdio.h>
#include <string.h>

#include "../../include/secure_functions.h"
#include "../security/hsm.h"
#include "cbor.h"

// FIDO2 specific instructions
#define INS_FIDO_MSG 0x10

// CTAP2 Commands
#define CTAP2_MAKE_CREDENTIAL 0x01
#define CTAP2_GET_ASSERTION 0x02
#define CTAP2_GET_INFO 0x04
#define CTAP2_CLIENT_PIN 0x06
#define CTAP2_RESET 0x07

void fido2_applet_init(void) {
  printf("FIDO2: Initializing applet...\n");
  fido2_storage_init();
}

static void handle_select(uint8_t *apdu_out, uint16_t *len_out) {
  // FIDO2 SELECT response returns fixed string "U2F_V2"
  const uint8_t response[] = "U2F_V2";
  memcpy(apdu_out, response, sizeof(response) - 1);
  iso7816_finalize_response(apdu_out, sizeof(response) - 1, len_out, SW_OK);
}

static void handle_get_info(uint8_t *apdu_out, uint16_t *len_out) {
  uint8_t buffer[256];
  uint8_t *ptr = buffer;

  // Status byte: 0x00 (Success)
  *ptr++ = 0x00;

  // CBOR map with 4 items: versions, extensions, aaguid, options
  ptr = cbor_encode_map(ptr, 4);

  // 0x01: versions -> ["FIDO_2_0", "U2F_V2"]
  ptr = cbor_encode_uint(ptr, 0x01);
  ptr = cbor_encode_array(ptr, 2);
  ptr = cbor_encode_text(ptr, "FIDO_2_0");
  ptr = cbor_encode_text(ptr, "U2F_V2");

  // 0x02: extensions -> []
  ptr = cbor_encode_uint(ptr, 0x02);
  ptr = cbor_encode_array(ptr, 0);

  // 0x03: aaguid -> 16 bytes (zeroed for now or fixed)
  static const uint8_t aaguid[16] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC,
                                     0xDE, 0xF0, 0x11, 0x22, 0x33, 0x44,
                                     0x55, 0x66, 0x77, 0x88};
  ptr = cbor_encode_uint(ptr, 0x03);
  ptr = cbor_encode_bytes(ptr, aaguid, 16);

  // 0x04: options -> {rk: true, up: true, uv: false}
  ptr = cbor_encode_uint(ptr, 0x04);
  ptr = cbor_encode_map(ptr, 3);
  ptr = cbor_encode_text(ptr, "rk");
  ptr = cbor_encode_bool(ptr, true);
  ptr = cbor_encode_text(ptr, "up");
  ptr = cbor_encode_bool(ptr, true);
  ptr = cbor_encode_text(ptr, "uv");
  ptr = cbor_encode_bool(ptr, false);

  uint16_t resp_len = ptr - buffer;
  memcpy(apdu_out, buffer, resp_len);
  iso7816_finalize_response(apdu_out, resp_len, len_out, SW_OK);
}

static uint8_t *encode_cose_key(uint8_t *ptr, const uint8_t *pubkey) {
  // COSE Map (5 items)
  ptr = cbor_encode_map(ptr, 5);

  // 1: kty -> 2 (EC2)
  ptr = cbor_encode_uint(ptr, 1);
  ptr = cbor_encode_uint(ptr, 2);

  // 3: alg -> -7 (ES256)
  ptr = cbor_encode_uint(ptr, 3);
  ptr = cbor_encode_nint(ptr, 6); // -1 - 6 = -7

  // -1: crv -> 1 (P-256)
  ptr = cbor_encode_nint(ptr, 0); // -1 - 0 = -1
  ptr = cbor_encode_uint(ptr, 1);

  // -2: x-coordinate (32 bytes)
  ptr = cbor_encode_nint(ptr, 1); // -1 - 1 = -2
  ptr = cbor_encode_bytes(ptr, pubkey, 32);

  // -3: y-coordinate (32 bytes)
  ptr = cbor_encode_nint(ptr, 2); // -1 - 2 = -3
  ptr = cbor_encode_bytes(ptr, pubkey + 32, 32);

  return ptr;
}

static void handle_make_credential(uint8_t *apdu_out, uint16_t *len_out) {
  printf("FIDO2: Real MakeCredential implementation\n");

  // For now, we use a fixed slot or dynamic slot from HSM
  uint8_t slot = 0; // Simplified for now
  if (hsm_generate_key(slot) != HSM_STATUS_OK) {
    uint8_t response[] = {0x21}; // CTAP2_ERR_NOT_ALLOWED
    memcpy(apdu_out, response, sizeof(response));
    iso7816_finalize_response(apdu_out, sizeof(response), len_out, SW_OK);
    return;
  }

  uint8_t pubkey[64];
  uint16_t pub_len;
  hsm_get_pubkey(slot, pubkey, &pub_len);

  // AuthData
  uint8_t auth_data[256];
  uint8_t *ad_ptr = auth_data;

  // RP ID Hash (32 bytes) - Stubbed for now
  memset(ad_ptr, 0, 32);
  ad_ptr += 32;

  // Flags: UP (1), AT (1) -> 0x41
  *ad_ptr++ = 0x41;

  // Sign Count (4 bytes)
  memset(ad_ptr, 0, 4);
  ad_ptr += 4;

  // AAGUID (16 bytes)
  static const uint8_t aaguid[16] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC,
                                     0xDE, 0xF0, 0x11, 0x22, 0x33, 0x44,
                                     0x55, 0x66, 0x77, 0x88};
  memcpy(ad_ptr, aaguid, 16);
  ad_ptr += 16;

  // Credential ID (16 bytes)
  uint8_t cred_id[16];
  for (int i = 0; i < 16; i++)
    cred_id[i] = (uint8_t)get_rand_32();
  *ad_ptr++ = 0x00; // Credential ID Length (16 bit)
  *ad_ptr++ = 16;
  memcpy(ad_ptr, cred_id, 16);
  ad_ptr += 16;

  // Save to storage
  fido2_storage_t *storage = fido2_storage_get_data();
  if (storage->count < FIDO2_MAX_CREDENTIALS) {
    fido2_credential_t *new_cred = &storage->credentials[storage->count];
    memcpy(new_cred->credential_id, cred_id, 16);
    new_cred->sign_count = 0;
    // Slot information is implicit or we can store it in a private_key field
    // (abuse) For now, let's just assume slot = index
    storage->count++;
    fido2_storage_save();
  }

  // Public Key (COSE)
  ad_ptr = encode_cose_key(ad_ptr, pubkey);

  uint16_t ad_len = ad_ptr - auth_data;

  // Final Response (CBOR)
  uint8_t response[512];
  uint8_t *resp_ptr = response;
  uint8_t *tmp_ptr;
  *resp_ptr++ = 0x00; // Success

  resp_ptr = cbor_encode_map(resp_ptr, 3);

  // fmt: "none"
  resp_ptr = cbor_encode_text(resp_ptr, "fmt");
  resp_ptr = cbor_encode_text(resp_ptr, "none");

  // authData
  resp_ptr = cbor_encode_text(resp_ptr, "authData");
  resp_ptr = cbor_encode_bytes(resp_ptr, auth_data, ad_len);

  // attStmt: {}
  resp_ptr = cbor_encode_text(resp_ptr, "attStmt");
  resp_ptr = cbor_encode_map(resp_ptr, 0);

  uint16_t final_len = resp_ptr - response;
  memcpy(apdu_out, response, final_len);
  iso7816_finalize_response(apdu_out, final_len, len_out, SW_OK);
}

static void handle_get_assertion(uint8_t *data, uint16_t len, uint8_t *apdu_out,
                                 uint16_t *len_out) {
  printf("FIDO2: Real GetAssertion implementation\n");

  cbor_parser_t parser = {.buffer = data + 1, .size = len - 1, .offset = 0};
  size_t map_size;
  if (!cbor_parse_map(&parser, &map_size)) {
    uint8_t response[] = {0x01}; // CTAP2_ERR_INVALID_COMMAND
    memcpy(apdu_out, response, sizeof(response));
    iso7816_finalize_response(apdu_out, sizeof(response), len_out, SW_OK);
    return;
  }

  const uint8_t *rp_id_hash = NULL;
  size_t rp_id_hash_len = 0;
  const uint8_t *client_data_hash = NULL;
  size_t client_data_hash_len = 0;

  // Simple key/value parsing in map
  for (size_t i = 0; i < map_size; i++) {
    uint64_t key;
    if (!cbor_parse_uint(&parser, &key))
      break;

    if (key == 0x01) { // rpId
      cbor_parse_bytes(&parser, &rp_id_hash, &rp_id_hash_len);
    } else if (key == 0x02) { // clientDataHash
      cbor_parse_bytes(&parser, &client_data_hash, &client_data_hash_len);
    } else {
      // Skip unknown keys or handled elsewhere
    }
  }

  // Find a credential (simplified: just take the first one for now)
  fido2_storage_t *storage = fido2_storage_get_data();
  if (storage->count == 0) {
    uint8_t response[] = {0x2E}; // CTAP2_ERR_NO_CREDENTIALS
    memcpy(apdu_out, response, sizeof(response));
    iso7816_finalize_response(apdu_out, sizeof(response), len_out, SW_OK);
    return;
  }

  // HSM Slot 0 assumed for now
  uint16_t sig_len = 0;
  uint8_t signature[64];

  // Real FIDO2 signs [AuthData || ClientDataHash]
  // For now, let's just sign the clientDataHash to show the mechanism is
  // working
  if (hsm_sign(0, client_data_hash, signature, &sig_len) != HSM_STATUS_OK) {
    uint8_t response[] = {0x01}; // Error
    memcpy(apdu_out, response, sizeof(response));
    iso7816_finalize_response(apdu_out, sizeof(response), len_out, SW_OK);
    return;
  }

  // AuthData for GetAssertion
  uint8_t auth_data[37];
  memcpy(auth_data, rp_id_hash, 32);
  auth_data[32] = 0x01;         // UP flag only
  memset(auth_data + 33, 0, 4); // signCount

  // Response (CBOR)
  uint8_t response[256];
  uint8_t *resp_ptr = response;
  *resp_ptr++ = 0x00; // Success

  resp_ptr = cbor_encode_map(resp_ptr, 3);

  // 1: credential
  resp_ptr = cbor_encode_uint(resp_ptr, 0x01);
  resp_ptr = cbor_encode_map(resp_ptr, 2);
  resp_ptr = cbor_encode_text(resp_ptr, "id");
  resp_ptr =
      cbor_encode_bytes(resp_ptr, storage->credentials[0].credential_id, 16);
  resp_ptr = cbor_encode_text(resp_ptr, "type");
  resp_ptr = cbor_encode_text(resp_ptr, "public-key");

  // 2: authData
  resp_ptr = cbor_encode_uint(resp_ptr, 0x02);
  resp_ptr = cbor_encode_bytes(resp_ptr, auth_data, 37);

  // 3: signature
  resp_ptr = cbor_encode_uint(resp_ptr, 0x03);
  resp_ptr = cbor_encode_bytes(resp_ptr, signature, 64);

  uint16_t final_len = resp_ptr - response;
  memcpy(apdu_out, response, final_len);
  iso7816_finalize_response(apdu_out, final_len, len_out, SW_OK);
}

static void handle_fido_msg(uint8_t *data, uint16_t lc, uint8_t *apdu_out,
                            uint16_t *len_out) {
  if (lc == 0) {
    iso7816_set_sw(apdu_out, len_out, SW_WRONG_LENGTH);
    return;
  }

  uint8_t cmd = data[0];
  printf("FIDO2: CTAP Command 0x%02X\n", cmd);

  switch (cmd) {
  case CTAP2_MAKE_CREDENTIAL: {
    handle_make_credential(apdu_out, len_out);
    break;
  }
  case CTAP2_GET_ASSERTION: {
    handle_get_assertion(data, lc, apdu_out, len_out);
    break;
  }
  case CTAP2_GET_INFO: {
    handle_get_info(apdu_out, len_out);
    break;
  }
  default:
    iso7816_set_sw(apdu_out, len_out, SW_FUNC_NOT_SUPPORTED);
    break;
  }
}

void fido2_applet_handle_msg(uint8_t *data_in, uint16_t len_in,
                             uint8_t *data_out, uint16_t *len_out) {
  handle_fido_msg(data_in, len_in, data_out, len_out);
}

void fido2_applet_handle_apdu(uint8_t *apdu_in, uint16_t len_in,
                              uint8_t *apdu_out, uint16_t *len_out) {
  uint8_t ins = apdu_in[APDU_INS_POS];
  uint8_t p1 = apdu_in[APDU_P1_POS];
  uint8_t p2 = apdu_in[APDU_P2_POS];

  printf("FIDO2: Handling APDU INS=0x%02X P1=0x%02X P2=0x%02X\n", ins, p1, p2);

  switch (ins) {
  case INS_SELECT:
    handle_select(apdu_out, len_out);
    break;

  case INS_FIDO_MSG: {
    uint16_t lc = (len_in > 4) ? apdu_in[APDU_LC_POS] : 0;
    handle_fido_msg(&apdu_in[APDU_DATA_POS], lc, apdu_out, len_out);
    break;
  }

  default:
    iso7816_set_sw(apdu_out, len_out, SW_INS_NOT_SUPPORTED);
    break;
  }
}
