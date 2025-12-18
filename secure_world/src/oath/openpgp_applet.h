#ifndef OPENPGP_APPLET_H
#define OPENPGP_APPLET_H

#include "../applet_manager.h"
#include "apdu_protocol.h"
#include <stdint.h>

void openpgp_applet_init(void);
void openpgp_applet_handle_apdu(uint8_t *apdu_in, uint16_t len_in,
                                uint8_t *apdu_out, uint16_t *len_out);

#endif // OPENPGP_APPLET_H
