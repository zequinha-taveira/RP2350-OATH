#ifndef LED_DRIVER_H
#define LED_DRIVER_H

#include <stdint.h>

void led_driver_init(void);

// Set color (0-255)
void led_set_color(uint8_t r, uint8_t g, uint8_t b);

#endif
