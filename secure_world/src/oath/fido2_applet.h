#ifndef FIDO2_APPLET_H
#define FIDO2_APPLET_H

#include "../applet_manager.h"
#include <stdint.h>

// FIDO2 AID: A0 00 00 06 47 2F 00 01
#define FIDO2_AID_LEN 8
static const uint8_t FIDO2_AID[FIDO2_AID_LEN] = {0xA0, 0x00, 0x00, 0x06,
                                                 0x47, 0x2F, 0x00, 0x01};

void fido2_applet_init(void);
void fido2_applet_handle_apdu(uint8_t *apdu_in, uint16_t len_in,
                              uint8_t *apdu_out, uint16_t *len_out);

#endif // FIDO2_APPLET_H
