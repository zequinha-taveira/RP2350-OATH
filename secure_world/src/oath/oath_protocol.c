#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <hardware/gpio.h>
#include <pico/stdlib.h>
#include <pico/time.h>

#include "../../../lib/libcotp/src/cotp.h"
#include "../drivers/led_driver.h"
#include "../time_sync.h"
#include "apdu_protocol.h"
#include "oath_protocol.h"
#include "oath_storage.h"

#define OATH_TOUCH_PIN 21

/**
 * @file oath_protocol.c
 * @brief Implementation of Yubico-compatible OATH protocol over CCID.
 */

static bool check_touch(void) {
  // Active low button on GPIO 21
  return !gpio_get(OATH_TOUCH_PIN);
}

void oath_init(void) {
  oath_storage_init();
  gpio_init(OATH_TOUCH_PIN);
  gpio_set_dir(OATH_TOUCH_PIN, GPIO_IN);
  gpio_pull_up(OATH_TOUCH_PIN);
  printf("[OATH] OATH Applet Initialized (Touch Pin: %d)\n", OATH_TOUCH_PIN);
}

void oath_handle_apdu(uint8_t *apdu_in, uint16_t len_in, uint8_t *apdu_out,
                      uint16_t *len_out) {
  if (len_in < 4) {
    *len_out = 0;
    return;
  }

  uint8_t ins = apdu_in[APDU_INS_POS];
  uint8_t p1 = apdu_in[APDU_P1_POS];

  // ISO SELECT (0xA4 with P1=04)
  if (ins == INS_SELECT && p1 == 0x04) {
    // SELECT OATH response: Tag 0x79 (Version) + Tag 0x71 (Name)
    uint8_t response[] = {0x79, 0x03, 0x05, 0x04, 0x03, // Version 5.4.3
                          0x71, 0x0b, 'Y',  'u',  'b',  'i', 'c',
                          'o',  ' ',  'O',  'A',  'T',  'H'};
    memcpy(apdu_out, response, sizeof(response));
    uint16_t response_len = sizeof(response);
    apdu_out[response_len++] = (uint8_t)(SW_OK >> 8);
    apdu_out[response_len++] = (uint8_t)(SW_OK & 0xFF);
    *len_out = response_len;
    return;
  }

  // OATH CALCULATE (0xA2)
  if (ins == INS_CALCULATE) {
    oath_credential_t cred;
    char name[64] = "RP2350-OATH:test@example.com"; // Placeholder lookup logic

    if (!oath_storage_get(name, &cred)) {
      apdu_out[0] = (uint8_t)(SW_FILE_NOT_FOUND >> 8);
      apdu_out[1] = (uint8_t)(SW_FILE_NOT_FOUND & 0xFF);
      *len_out = 2;
      return;
    }

    if (cred.touch_required && !check_touch()) {
      apdu_out[0] = (uint8_t)(SW_CONDITIONS_NOT_SATISFIED >> 8);
      apdu_out[1] = (uint8_t)(SW_CONDITIONS_NOT_SATISFIED & 0xFF);
      *len_out = 2;
      return;
    }

    cotp_error_t err;
    char b32_secret[128];
    if (!base32_encode_buf(cred.secret, cred.secret_len, b32_secret,
                           sizeof(b32_secret), &err)) {
      apdu_out[0] = (uint8_t)(SW_UNKNOWN >> 8);
      apdu_out[1] = (uint8_t)(SW_UNKNOWN & 0xFF);
      *len_out = 2;
      return;
    }

    char otp[16];
    bool success = false;
    if (cred.type == OATH_TYPE_TOTP) {
      uint64_t ts = time_sync_get_timestamp();
      success = get_totp_at_buf(b32_secret, (long)ts, cred.digits, 30, SHA1,
                                otp, sizeof(otp), &err);
    } else {
      success = get_hotp_buf(b32_secret, cred.counter, cred.digits, SHA1, otp,
                             sizeof(otp), &err);
      if (success)
        oath_storage_update_counter(name, cred.counter + 1);
    }

    if (success) {
      size_t o_len = strlen(otp);
      apdu_out[0] = 0x76;
      apdu_out[1] = (uint8_t)o_len;
      memcpy(&apdu_out[2], otp, o_len);
      uint16_t response_len = (uint16_t)(o_len + 2);
      apdu_out[response_len++] = (uint8_t)(SW_OK >> 8);
      apdu_out[response_len++] = (uint8_t)(SW_OK & 0xFF);
      *len_out = response_len;
    } else {
      apdu_out[0] = (uint8_t)(SW_UNKNOWN >> 8);
      apdu_out[1] = (uint8_t)(SW_UNKNOWN & 0xFF);
      *len_out = 2;
    }
    return;
  }

  // OATH LIST (0xA1)
  if (ins == INS_LIST) {
    uint16_t offset = 0;
    for (uint32_t i = 0; i < MAX_CREDENTIALS; i++) {
      const char *name = oath_storage_list(i);
      if (name) {
        size_t n_len = strlen(name);
        apdu_out[offset++] = 0x71; // Name Tag
        apdu_out[offset++] = (uint8_t)n_len;
        memcpy(apdu_out + offset, name, n_len);
        offset += n_len;
      }
    }
    apdu_out[offset++] = (uint8_t)(SW_OK >> 8);
    apdu_out[offset++] = (uint8_t)(SW_OK & 0xFF);
    *len_out = offset;
    return;
  }

  // OATH CALCULATE ALL (0xA4 with P1=0x00)
  if (ins == INS_CALCULATE_ALL && p1 == 0x00) {
    uint16_t offset = 0;
    cotp_error_t err;
    char otp[16];
    uint64_t ts = time_sync_get_timestamp();

    for (uint32_t i = 0; i < MAX_CREDENTIALS; i++) {
      const char *name = oath_storage_list(i);
      if (name) {
        oath_credential_t cred;
        if (oath_storage_get(name, &cred)) {
          size_t n_len = strlen(name);
          apdu_out[offset++] = 0x71; // Name Tag
          apdu_out[offset++] = (uint8_t)n_len;
          memcpy(apdu_out + offset, name, n_len);
          offset += n_len;

          char b32_secret[128];
          if (base32_encode_buf(cred.secret, cred.secret_len, b32_secret,
                                sizeof(b32_secret), &err)) {
            bool success = false;
            if (cred.type == OATH_TYPE_TOTP) {
              success = get_totp_at_buf(b32_secret, (long)ts, cred.digits, 30,
                                        SHA1, otp, sizeof(otp), &err);
            } else {
              success = get_hotp_buf(b32_secret, cred.counter, cred.digits,
                                     SHA1, otp, sizeof(otp), &err);
            }

            if (success) {
              size_t o_len = strlen(otp);
              apdu_out[offset++] = 0x76; // Truncated Response Tag
              apdu_out[offset++] = (uint8_t)o_len;
              memcpy(apdu_out + offset, otp, o_len);
              offset += o_len;
            }
          }
        }
      }
    }
    apdu_out[offset++] = (uint8_t)(SW_OK >> 8);
    apdu_out[offset++] = (uint8_t)(SW_OK & 0xFF);
    *len_out = offset;
    return;
  }

  // Placeholder for other OATH commands
  apdu_out[0] = (uint8_t)(SW_INS_NOT_SUPPORTED >> 8);
  apdu_out[1] = (uint8_t)(SW_INS_NOT_SUPPORTED & 0xFF);
  *len_out = 2;
}
