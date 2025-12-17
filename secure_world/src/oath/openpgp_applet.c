#include "openpgp_applet.h"
#include "apdu_protocol.h"
#include "iso7816_4.h"
#include "openpgp_storage.h"
#include <stdio.h>
#include <string.h>

// OpenPGP specific instructions
#define INS_GET_DATA 0xCA
#define INS_PUT_DATA 0xDA
#define INS_VERIFY 0x20

// OpenPGP Data Objects (DOs)
#define DO_AID 0x004F
#define DO_LOGIN 0x005E     // Login Data
#define DO_CRD 0x0065       // Cardholder Related Data
#define DO_ARD 0x006E       // Application Related Data
#define DO_PW_STATUS 0x00C4 // PW Status Bytes

// PIN IDs
#define PW1 0x81
#define PW1_83 0x83
#define PW3 0x82

void openpgp_applet_init(void) {
  printf("OpenPGP: Initializing applet...\n");
  openpgp_storage_init();
}

static void handle_select(uint8_t *apdu_out, uint16_t *len_out) {
  // Return Application Related Data (ARD) in SELECT response
  openpgp_data_t *pgp_data = openpgp_storage_get_data();
  uint8_t response[32];
  uint16_t pos = 0;

  // ARD Template (0x6E)
  response[pos++] = 0x6E;
  response[pos++] = 10; // Simple fixed length for mock

  // AID subset (0x4F)
  response[pos++] = 0x4F;
  response[pos++] = 6;
  memcpy(&response[pos], OPENPGP_AID, 6);
  pos += 6;

  // Remaining basic info...
  memcpy(apdu_out, response, pos);
  iso7816_finalize_response(apdu_out, pos, len_out, SW_OK);
}

static void handle_verify(uint8_t p1, uint8_t p2, uint8_t *data, uint16_t lc,
                          uint8_t *apdu_out, uint16_t *len_out) {
  openpgp_data_t *pgp_data = openpgp_storage_get_data();
  printf("OpenPGP: VERIFY for PW 0x%02X\n", p2);

  if (p1 == 0xFF) {
    // Reset/Logout
    printf("OpenPGP: Logout for PW 0x%02X\n", p2);
    iso7816_set_sw(apdu_out, len_out, SW_OK);
    return;
  }

  // Placeholder for real PIN check
  // For now, accept anything as mock (Success: 90 00)
  printf("OpenPGP: PIN check (Mock Success)\n");
  iso7816_set_sw(apdu_out, len_out, SW_OK);
}

static void handle_get_data(uint8_t p1, uint8_t p2, uint8_t *apdu_out,
                            uint16_t *len_out) {
  uint16_t tag = (p1 << 8) | p2;
  openpgp_data_t *pgp_data = openpgp_storage_get_data();
  printf("OpenPGP: GET DATA for Tag 0x%04X\n", tag);

  switch (tag) {
  case DO_AID:
    memcpy(apdu_out, OPENPGP_AID, OPENPGP_AID_LEN);
    iso7816_finalize_response(apdu_out, OPENPGP_AID_LEN, len_out, SW_OK);
    break;

  case DO_PW_STATUS: {
    // PW Status Bytes: [0] Forcing PIN, [1] User PIN length, [2] Admin PIN
    // length, [3] PW1 retry, [4] PW2 retry (N/A), [5] PW3 retry
    uint8_t status[] = {0x00, 0x08,
                        0x08, pgp_data->pin_retry_counter[0],
                        0x00, pgp_data->pin_retry_counter[2]};
    memcpy(apdu_out, status, sizeof(status));
    iso7816_finalize_response(apdu_out, sizeof(status), len_out, SW_OK);
    break;
  }

  case DO_LOGIN: {
    uint16_t name_len = strlen(pgp_data->name);
    memcpy(apdu_out, pgp_data->name, name_len);
    iso7816_finalize_response(apdu_out, name_len, len_out, SW_OK);
    break;
  }

  case DO_CRD: {
    // Cardholder Related Data (Mock Template)
    uint8_t crd[] = {
        0x65, 0x04, 0x5B, 0x02, 'e', 'n' // Language
    };
    memcpy(apdu_out, crd, sizeof(crd));
    iso7816_finalize_response(apdu_out, sizeof(crd), len_out, SW_OK);
    break;
  }

  case DO_ARD: {
    // Application Related Data Template
    uint8_t ard[64];
    uint16_t pos = 0;
    ard[pos++] = 0x6E;
    ard[pos++] = 20; // Length

    ard[pos++] = 0x4F; // AID
    ard[pos++] = 6;
    memcpy(&ard[pos], OPENPGP_AID, 6);
    pos += 6;

    ard[pos++] = 0x73; // Discretionary data
    ard[pos++] = 4;
    ard[pos++] = 0xC0;
    ard[pos++] = 0x01;
    ard[pos++] = 0x01; // Mock caps

    memcpy(apdu_out, ard, pos);
    iso7816_finalize_response(apdu_out, pos, len_out, SW_OK);
    break;
  }

  default:
    iso7816_set_sw(apdu_out, len_out, SW_FILE_NOT_FOUND);
    break;
  }
}

void openpgp_applet_handle_apdu(uint8_t *apdu_in, uint16_t len_in,
                                uint8_t *apdu_out, uint16_t *len_out) {
  uint8_t ins = apdu_in[APDU_INS_POS];
  uint8_t p1 = apdu_in[APDU_P1_POS];
  uint8_t p2 = apdu_in[APDU_P2_POS];

  printf("OpenPGP: Handling APDU INS=0x%02X P1=0x%02X P2=0x%02X\n", ins, p1,
         p2);

  switch (ins) {
  case INS_SELECT:
    handle_select(apdu_out, len_out);
    break;

  case INS_GET_DATA:
    handle_get_data(p1, p2, apdu_out, len_out);
    break;

  case INS_VERIFY: {
    uint8_t lc = (len_in > 4) ? apdu_in[APDU_LC_POS] : 0;
    handle_verify(p1, p2, &apdu_in[APDU_DATA_POS], lc, apdu_out, len_out);
    break;
  }

  default:
    iso7816_set_sw(apdu_out, len_out, SW_INS_NOT_SUPPORTED);
    break;
  }
}
