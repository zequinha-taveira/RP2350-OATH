#include "iso7816_4.h"
#include <stdio.h>
#include <string.h>


void iso7816_set_sw(uint8_t *buffer, uint16_t *len, uint16_t sw) {
  buffer[0] = (uint8_t)(sw >> 8);
  buffer[1] = (uint8_t)(sw & 0xFF);
  *len = 2;
}

void iso7816_finalize_response(uint8_t *buffer, uint16_t data_len,
                               uint16_t *total_len, uint16_t sw) {
  buffer[data_len] = (uint8_t)(sw >> 8);
  buffer[data_len + 1] = (uint8_t)(sw & 0xFF);
  *total_len = data_len + 2;
}
