#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>


#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "ws2812.pio.h"

#define IS_RGBW false
#define NUM_PIXELS 1

// Default Pin, can be overridden
#ifndef WS2812_PIN
#define WS2812_PIN 22
#endif

static PIO pio_instance = pio0; // Use PIO0
static int sm = 0;

static inline void put_pixel(uint32_t pixel_grb) {
  pio_sm_put_blocking(pio_instance, sm, pixel_grb << 8u);
}

static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b) {
  return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

void led_driver_init(void) {
  // Basic PIO init
  // Find free SM? For now hardcode.
  uint offset = pio_add_program(pio_instance, &ws2812_program);
  ws2812_program_init(pio_instance, sm, offset, WS2812_PIN, 800000, IS_RGBW);
}

void led_set_color(uint8_t r, uint8_t g, uint8_t b) {
  put_pixel(urgb_u32(r, g, b));
}
