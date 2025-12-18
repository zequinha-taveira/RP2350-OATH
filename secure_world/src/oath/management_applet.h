#ifndef MANAGEMENT_APPLET_H
#define MANAGEMENT_APPLET_H

#include "../applet_manager.h"
#include "apdu_protocol.h"
#include <stdint.h>

void management_applet_init(void);
void management_applet_handle_apdu(uint8_t *apdu_in, uint16_t len_in,
                                   uint8_t *apdu_out, uint16_t *len_out);

#endif // MANAGEMENT_APPLET_H
