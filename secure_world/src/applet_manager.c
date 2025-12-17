#include "applet_manager.h"
#include "oath/apdu_protocol.h"
#include "oath/oath_protocol.h"
#include <stdio.h>
#include <string.h>


// Forward declarations of applet handlers (placeholders for now)
void openpgp_handle_apdu(uint8_t *apdu_in, uint16_t len_in, uint8_t *apdu_out,
                         uint16_t *len_out);
void openpgp_init(void);
void fido2_handle_apdu(uint8_t *apdu_in, uint16_t len_in, uint8_t *apdu_out,
                       uint16_t *len_out);
void fido2_init(void);

// OpenPGP AID: D2 76 00 01 24 01
static const uint8_t OPENPGP_AID[] = {0xD2, 0x76, 0x00, 0x01, 0x24, 0x01};

static const applet_t applets[] = {
    {.aid = {0xA0, 0x00, 0x00, 0x05, 0x27, 0x20, 0x01},
     .aid_len = 7,
     .handle_apdu = oath_handle_apdu,
     .init = oath_init,
     .name = "OATH"},
    {.aid = {0xD2, 0x76, 0x00, 0x01, 0x24, 0x01},
     .aid_len = 6,
     .handle_apdu = openpgp_handle_apdu, // placeholder
     .init = openpgp_init,               // placeholder
     .name = "OpenPGP"},
    {.aid = {0xA0, 0x00, 0x00, 0x06, 0x47, 0x2F, 0x00, 0x01},
     .aid_len = 8,
     .handle_apdu = fido2_handle_apdu, // placeholder
     .init = fido2_init,               // placeholder
     .name = "FIDO2"}};

#define APPLET_COUNT (sizeof(applets) / sizeof(applets[0]))

static int active_applet_idx = -1;

void applet_manager_init(void) {
  printf("AppletManager: Initializing...\n");
  for (int i = 0; i < APPLET_COUNT; i++) {
    if (applets[i].init) {
      applets[i].init();
    }
  }
  active_applet_idx = -1;
}

static void send_sw(uint16_t sw, uint8_t *apdu_out, uint16_t *len_out) {
  apdu_out[0] = (uint8_t)(sw >> 8);
  apdu_out[1] = (uint8_t)(sw & 0xFF);
  *len_out = 2;
}

void applet_manager_handle_apdu(uint8_t *apdu_in, uint16_t len_in,
                                uint8_t *apdu_out, uint16_t *len_out) {
  if (len_in < 5) {
    send_sw(SW_WRONG_LENGTH, apdu_out, len_out);
    return;
  }

  uint8_t ins = apdu_in[APDU_INS_POS];
  uint8_t p1 = apdu_in[APDU_P1_POS];

  if (ins == INS_SELECT && p1 == 0x04) {
    // AID selection
    uint8_t lc = apdu_in[APDU_LC_POS];
    uint8_t *aid = apdu_in + APDU_DATA_POS;

    for (int i = 0; i < APPLET_COUNT; i++) {
      if (lc == applets[i].aid_len && memcmp(aid, applets[i].aid, lc) == 0) {
        active_applet_idx = i;
        printf("AppletManager: Selected %s\n", applets[i].name);
        applets[i].handle_apdu(apdu_in, len_in, apdu_out, len_out);
        return;
      }
    }

    printf("AppletManager: Applet not found\n");
    active_applet_idx = -1;
    send_sw(SW_FILE_NOT_FOUND, apdu_out, len_out);
    return;
  }

  if (active_applet_idx != -1) {
    applets[active_applet_idx].handle_apdu(apdu_in, len_in, apdu_out, len_out);
  } else {
    printf("AppletManager: No applet selected\n");
    send_sw(SW_SECURITY_STATUS_NOT_SAT, apdu_out, len_out);
  }
}

// Temporary placeholders for new applets
void openpgp_handle_apdu(uint8_t *apdu_in, uint16_t len_in, uint8_t *apdu_out,
                         uint16_t *len_out) {
  printf("OpenPGP: Handle APDU (stub)\n");
  send_sw(SW_OK, apdu_out, len_out);
}

void openpgp_init(void) { printf("OpenPGP: Init\n"); }

void fido2_handle_apdu(uint8_t *apdu_in, uint16_t len_in, uint8_t *apdu_out,
                       uint16_t *len_out) {
  printf("FIDO2: Handle APDU (stub)\n");
  send_sw(SW_OK, apdu_out, len_out);
}

void fido2_init(void) { printf("FIDO2: Init\n"); }
