#include "oath_protocol.h"
#include "apdu_protocol.h"
#include "cotp.h"
#include "drivers/led_driver.h"
#include "oath_storage.h"
#include "time_sync.h"
#include <hardware/gpio.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @file oath_protocol.c
 * @brief Implementation of Yubico-compatible OATH protocol over CCID.
 */

// Global state
static bool session_unlocked = false;
static uint32_t last_touch_ack_ms = 0;

// Hardware configuration
#define TOUCH_BUTTON_PIN 21

//--------------------------------------------------------------------+
// Protocol Helpers
//--------------------------------------------------------------------+

static void send_sw(uint16_t sw, uint8_t *apdu_out, uint16_t *len_out) {
  apdu_out[0] = (uint8_t)(sw >> 8);
  apdu_out[1] = (uint8_t)(sw & 0xFF);
  *len_out = 2;
}

static bool do_touch_check(void) {
  printf("[OATH] User Presence Test (UPT) required. Waiting for touch...\n");

  // Simulation of User Presence verification (Button GP21)
  // In a production device, this would be integrated into the main loop
  // to keep the USB stack alive. Here we use a limited loop.
  for (int i = 0; i < 100; i++) {
    led_set_color(0, 0, 255); // Pulsing Blue
    sleep_ms(25);
    led_set_color(0, 0, 0);
    sleep_ms(25);

    if (!gpio_get(TOUCH_BUTTON_PIN)) {
      printf("[OATH] User Presence Confirmed.\n");
      led_set_color(0, 255, 0); // Success Green
      sleep_ms(100);
      return true;
    }
  }

  printf("[OATH] UPT Timeout. Request denied.\n");
  led_set_color(255, 0, 0); // Failure Red
  sleep_ms(200);
  return false;
}

//--------------------------------------------------------------------+
// Command Handlers
//--------------------------------------------------------------------+

static void handle_select(uint8_t *apdu_out, uint16_t *len_out) {
  // Response for OATH SELECT: Tag 0x79 (Version)
  // Ensure we don't overflow (need at least 7 bytes)
  apdu_out[0] = 0x79;
  apdu_out[1] = 0x03;
  apdu_out[2] = 0x05; // v5.4.3
  apdu_out[3] = 0x04;
  apdu_out[4] = 0x03;
  *len_out = 5;
  send_sw(SW_OK, apdu_out + 5, len_out);
  *len_out = 7; // Fixed length for this simple response
}

static void handle_put(uint8_t *data, uint16_t len, uint8_t *apdu_out,
                       uint16_t *len_out) {
  if (oath_storage_is_password_set() && !session_unlocked) {
    send_sw(SW_SECURITY_STATUS_NOT_SAT, apdu_out, len_out);
    return;
  }

  if (len < 4 || data[0] != 0x71) {
    send_sw(SW_WRONG_DATA, apdu_out, len_out);
    return;
  }

  uint16_t offset = 0;

  // Parse Name (0x71)
  offset++;
  uint8_t name_len = data[offset++];
  if (offset + name_len > len) {
    send_sw(SW_WRONG_DATA, apdu_out, len_out);
    return;
  }

  char name[OATH_MAX_NAME_LEN];
  uint8_t copy_len =
      (name_len < OATH_MAX_NAME_LEN) ? name_len : (OATH_MAX_NAME_LEN - 1);
  memcpy(name, &data[offset], copy_len);
  name[copy_len] = '\0';
  offset += name_len;

  // Parse Key (0x73)
  if (offset + 2 > len || data[offset++] != 0x73) {
    send_sw(SW_WRONG_DATA, apdu_out, len_out);
    return;
  }

  uint8_t key_block_len = data[offset++];
  if (offset + key_block_len > len || key_block_len < 2) {
    send_sw(SW_WRONG_DATA, apdu_out, len_out);
    return;
  }

  uint8_t type_ptr = data[offset++];
  uint8_t digits = data[offset++];
  uint8_t *secret = &data[offset];
  uint8_t secret_len = key_block_len - 2;

  oath_type_t type =
      ((type_ptr & 0xF0) == 0x10) ? OATH_TYPE_HOTP : OATH_TYPE_TOTP;
  oath_algo_t algo = (oath_algo_t)(type_ptr & 0x0F);

  if (oath_storage_put(name, secret, secret_len, type, algo, digits, 30, 0)) {
    send_sw(SW_OK, apdu_out, len_out);
  } else {
    send_sw(SW_MEMORY_FAILURE, apdu_out, len_out);
  }
}

static void handle_calculate(uint8_t *data, uint16_t len, uint8_t *apdu_out,
                             uint16_t *len_out) {
  if (len < 2 || data[0] != 0x71) {
    send_sw(SW_WRONG_DATA, apdu_out, len_out);
    return;
  }

  uint8_t name_len = data[1];
  if (name_len + 2 > len) {
    send_sw(SW_WRONG_DATA, apdu_out, len_out);
    return;
  }

  char name[OATH_MAX_NAME_LEN];
  uint8_t copy_len =
      (name_len < OATH_MAX_NAME_LEN) ? name_len : (OATH_MAX_NAME_LEN - 1);
  memcpy(name, &data[2], copy_len);
  name[copy_len] = '\0';

  oath_credential_t cred;
  if (!oath_storage_get(name, &cred)) {
    send_sw(SW_FILE_NOT_FOUND, apdu_out, len_out);
    return;
  }

  if (cred.touch_required && !do_touch_check()) {
    send_sw(SW_CONDITIONS_NOT_SATISFIED, apdu_out, len_out);
    return;
  }

  // Logic for generating OTP
  cotp_error_t err;
  char *base32_s = base32_encode(cred.secret, cred.secret_len, &err);
  if (!base32_s) {
    send_sw(SW_UNKNOWN, apdu_out, len_out);
    return;
  }

  char *otp = NULL;
  if (cred.type == OATH_TYPE_TOTP) {
    uint64_t ts = time_sync_get_timestamp();
    otp = get_totp_at(base32_s, (long)ts, cred.digits, 30, SHA1, &err);
  } else {
    otp = get_hotp(base32_s, cred.counter, cred.digits, SHA1, &err);
    oath_storage_update_counter(name, cred.counter + 1);
  }

  free(base32_s);

  if (otp) {
    size_t o_len = strlen(otp);
    // Safety check: ensure we don't overflow. Max response buffer is usually
    // handled at the gateway, but we check here for defense-in-depth.
    if (o_len > 16) { // TOTP/HOTP codes are normally 6-8 digits
      free(otp);
      send_sw(SW_UNKNOWN, apdu_out, len_out);
      return;
    }

    apdu_out[0] = 0x76;
    apdu_out[1] = (uint8_t)o_len;
    memcpy(&apdu_out[2], otp, o_len);
    free(otp);
    uint16_t response_len = (uint16_t)(o_len + 2);
    send_sw(SW_OK, apdu_out + response_len, len_out);
    *len_out = response_len + 2;
  } else {
    send_sw(SW_UNKNOWN, apdu_out, len_out);
  }
}

//--------------------------------------------------------------------+
// Main Entry Points
//--------------------------------------------------------------------+

void oath_init(void) {
  printf("[OATH] Initializing Secure OATH Protocol Handler...\n");
  oath_storage_init();
  gpio_init(TOUCH_BUTTON_PIN);
  gpio_set_dir(TOUCH_BUTTON_PIN, GPIO_IN);
  gpio_pull_up(TOUCH_BUTTON_PIN);
}

void oath_handle_apdu(uint8_t *apdu_in, uint16_t len_in, uint8_t *apdu_out,
                      uint16_t *len_out) {
  if (len_in < 4) {
    send_sw(SW_WRONG_LENGTH, apdu_out, len_out);
    return;
  }

  uint8_t ins = apdu_in[APDU_INS_POS];
  uint8_t p1 = apdu_in[APDU_P1_POS];
  uint8_t *data = &apdu_in[APDU_DATA_POS];
  uint16_t data_len = (len_in > 4) ? apdu_in[APDU_LC_POS] : 0;

  switch (ins) {
  case INS_SELECT:
    handle_select(apdu_out, len_out);
    break;

  case 0x01: // PUT
    handle_put(data, data_len, apdu_out, len_out);
    break;

  case 0xA1: // CALCULATE
    handle_calculate(data, data_len, apdu_out, len_out);
    break;

  case 0x04: // RESET
    oath_storage_reset();
    send_sw(SW_OK, apdu_out, len_out);
    break;

  default:
    send_sw(SW_INS_NOT_SUPPORTED, apdu_out, len_out);
    break;
  }
}
