#ifndef OPENPGP_APPLET_H
#define OPENPGP_APPLET_H

#include "../applet_manager.h"
#include <stdint.h>

// OpenPGP AID: D2 76 00 01 24 01
#define OPENPGP_AID_LEN 6
static const uint8_t OPENPGP_AID[OPENPGP_AID_LEN] = {0xD2, 0x76, 0x00,
                                                     0x01, 0x24, 0x01};

void openpgp_applet_init(void);
void openpgp_applet_handle_apdu(uint8_t *apdu_in, uint16_t len_in,
                                uint8_t *apdu_out, uint16_t *len_out);

#endif // OPENPGP_APPLET_H
