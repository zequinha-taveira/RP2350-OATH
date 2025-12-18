#ifndef FIDO2_APPLET_H
#define FIDO2_APPLET_H

#include "../applet_manager.h"
#include "apdu_protocol.h"
#include <stdint.h>

void fido2_applet_init(void);
void fido2_applet_handle_apdu(uint8_t *apdu_in, uint16_t len_in,
                              uint8_t *apdu_out, uint16_t *len_out);
void fido2_applet_handle_msg(uint8_t *data_in, uint16_t len_in,
                             uint8_t *data_out, uint16_t *len_out);

#endif // FIDO2_APPLET_H
