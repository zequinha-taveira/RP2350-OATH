#ifndef OATH_PROTOCOL_H
#define OATH_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>

// Initialize the OATH protocol handler and storage
void oath_init(void);

// Handle APDU commands directed to the OATH application
void oath_handle_apdu(uint8_t *apdu_in, uint16_t len_in, uint8_t *apdu_out, uint16_t *len_out);

#endif // OATH_PROTOCOL_H
