#include "fido2_applet.h"
#include "apdu_protocol.h"
#include "fido2_storage.h"
#include "iso7816_4.h"
#include <stdio.h>
#include <string.h>


// FIDO2 specific instructions
#define INS_FIDO_MSG 0x10

void fido2_applet_init(void) {
  printf("FIDO2: Initializing applet...\n");
  fido2_storage_init();
}

static void handle_select(uint8_t *apdu_out, uint16_t *len_out) {
  // FIDO2 SELECT response is typically empty or returns "U2F_V2"
  const uint8_t response[] = "U2F_V2";
  memcpy(apdu_out, response, sizeof(response) - 1);
  iso7816_finalize_response(apdu_out, sizeof(response) - 1, len_out, SW_OK);
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
  case 0x01: { // AuthenticatorMakeCredential
    printf("FIDO2: MakeCredential called (Stub)\n");
    // Placeholder: Return a mock error for now (NOT_ALLOWED)
    uint8_t response[] = {0x21}; // CTAP2_ERR_NOT_ALLOWED
    memcpy(apdu_out, response, sizeof(response));
    iso7816_finalize_response(apdu_out, sizeof(response), len_out, SW_OK);
    break;
  }
  case 0x04: { // AuthenticatorGetInfo
    // hardcoded CBOR response for AuthenticatorGetInfo
    static const uint8_t info_response[] = {0x00, // Success
                                            0xA1, 0x01, 0x81, 0x68, 'F', 'I',
                                            'D',  'O',  '_',  '2',  '_', '0'};
    memcpy(apdu_out, info_response, sizeof(info_response));
    iso7816_finalize_response(apdu_out, sizeof(info_response), len_out, SW_OK);
    break;
  }
  default:
    iso7816_set_sw(apdu_out, len_out, SW_FUNC_NOT_SUPPORTED);
    break;
  }
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
