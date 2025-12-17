#include "applet_manager.h"
#include "oath/apdu_protocol.h"
#include "oath/fido2_applet.h"
#include "oath/oath_protocol.h"
#include "oath/openpgp_applet.h"
#include <stdio.h>
#include <string.h>

// Max number of registered applets
#define MAX_APPLETS 5

static secure_applet_t registered_applets[MAX_APPLETS];
static uint8_t num_applets = 0;
static secure_applet_t *selected_applet = NULL;

void applet_manager_init(void) {
  printf("Applet Manager: Initializing...\n");
  num_applets = 0;
  selected_applet = NULL;

  // Register OATH Applet
  registered_applets[num_applets].aid = OATH_AID;
  registered_applets[num_applets].aid_len = OATH_AID_LEN;
  registered_applets[num_applets].init = oath_init;
  registered_applets[num_applets].handle_apdu = oath_handle_apdu;
  num_applets++;

  // Register OpenPGP Applet
  registered_applets[num_applets].aid = OPENPGP_AID;
  registered_applets[num_applets].aid_len = OPENPGP_AID_LEN;
  registered_applets[num_applets].init = openpgp_applet_init;
  registered_applets[num_applets].handle_apdu = openpgp_applet_handle_apdu;
  num_applets++;

  // Register FIDO2 Applet
  registered_applets[num_applets].aid = FIDO2_AID;
  registered_applets[num_applets].aid_len = FIDO2_AID_LEN;
  registered_applets[num_applets].init = fido2_applet_init;
  registered_applets[num_applets].handle_apdu = fido2_applet_handle_apdu;
  num_applets++;

  // Initialize all applets
  for (int i = 0; i < num_applets; i++) {
    if (registered_applets[i].init) {
      registered_applets[i].init();
    }
  }
}

void applet_manager_process_apdu(uint8_t *apdu_in, uint16_t len_in,
                                 uint8_t *apdu_out, uint16_t *len_out) {
  if (len_in < 5) {
    // Return SW_WRONG_LENGTH (67 00)
    apdu_out[0] = 0x67;
    apdu_out[1] = 0x00;
    *len_out = 2;
    return;
  }

  uint8_t ins = apdu_in[APDU_INS_POS];

  // Handle SELECT command for AID switching
  if (ins == INS_SELECT) {
    uint8_t lc = apdu_in[APDU_LC_POS];
    uint8_t *target_aid = &apdu_in[APDU_DATA_POS];

    for (int i = 0; i < num_applets; i++) {
      if (lc == registered_applets[i].aid_len &&
          memcmp(target_aid, registered_applets[i].aid, lc) == 0) {

        selected_applet = &registered_applets[i];
        printf("Applet Manager: Switched to applet %d\n", i);

        // Allow the applet to process its own SELECT response
        selected_applet->handle_apdu(apdu_in, len_in, apdu_out, len_out);
        return;
      }
    }

    // AID not found
    apdu_out[0] = 0x6A;
    apdu_out[1] = 0x82; // SW_FILE_NOT_FOUND
    *len_out = 2;
    return;
  }

  // Delegate to selected applet
  if (selected_applet) {
    selected_applet->handle_apdu(apdu_in, len_in, apdu_out, len_out);
  } else {
    // No applet selected
    apdu_out[0] = 0x69;
    apdu_out[1] = 0x85; // SW_CONDITIONS_NOT_SATISFIED
    *len_out = 2;
  }
}
