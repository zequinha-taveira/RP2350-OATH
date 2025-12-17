#include "oath_protocol.h"
#include "apdu_protocol.h" // For APDU and OATH constants
#include "cotp.h"          // Using libcotp for core logic
#include "drivers/led_driver.h"
#include "oath_storage.h"
#include "time_sync.h"
#include <hardware/gpio.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global state for the OATH application
static uint32_t last_list_index = 0;  // For paged listing
static bool session_unlocked = false; // Tracks if password has been validated
static uint64_t last_touch_request = 0;

// Hardware configuration (To be moved to board config)
#define TOUCH_BUTTON_PIN 21 // Updated to GP21 (External)

static bool do_touch_check(void) {
  // Blink LED to request touch
  printf("OATH: Waiting for user touch (GP21)...\n");

  // Simple timeout loop (e.g., 5 seconds)
  for (int i = 0; i < 50; i++) {
    led_set_color(255, 255, 0); // Yellow
    sleep_ms(50);
    led_set_color(0, 0, 0); // Off
    sleep_ms(50);

    // Check Button (Active Low assumes pullup)
    if (!gpio_get(TOUCH_BUTTON_PIN)) {
      led_set_color(0, 255, 0); // Green
      sleep_ms(200);
      return true;
    }
  }

  led_set_color(255, 0, 0); // Red (Timeout)
  sleep_ms(500);
  led_set_color(0, 0, 0);
  return false;
}

static void send_sw(uint16_t sw, uint8_t *apdu_out, uint16_t *len_out) {
  apdu_out[0] = (uint8_t)(sw >> 8);
  apdu_out[1] = (uint8_t)(sw & 0xFF);
  *len_out = 2;
}

// Handler for the ISO 7816 SELECT command (INS=A4)
static void handle_select_apdu(uint8_t *apdu_in, uint16_t len_in,
                               uint8_t *apdu_out, uint16_t *len_out) {
  // Applet Manager already verified the AID.
  // Standard response for OATH SELECT is version info (3 bytes) + SW_OK
  apdu_out[0] = 0x79; // Version tag
  apdu_out[1] = 0x03; // length
  apdu_out[2] = 0x05; // Major
  apdu_out[3] = 0x04; // Minor
  apdu_out[4] = 0x03; // Patch
  *len_out = 5;
  send_sw(SW_OK, apdu_out + 5, len_out);
  *len_out += 5;
}

// Handler for PUT Command (0x01)
static void handle_put_command(uint8_t *data, uint16_t len, uint8_t *apdu_out,
                               uint16_t *len_out) {
  uint16_t offset = 0;

  // Access Control
  if (oath_storage_is_password_set() && !session_unlocked) {
    send_sw(SW_SECURITY_STATUS_NOT_SAT, apdu_out, len_out);
    return;
  }

  // Parse Name
  if (data[offset++] != 0x71) {
    send_sw(SW_WRONG_DATA, apdu_out, len_out);
    return;
  }
  uint8_t name_len = data[offset++];
  if (offset + name_len > len) {
    send_sw(SW_WRONG_DATA, apdu_out, len_out);
    return;
  }

  char name[OATH_MAX_NAME_LEN];
  if (name_len >= OATH_MAX_NAME_LEN)
    name_len = OATH_MAX_NAME_LEN - 1;
  memcpy(name, &data[offset], name_len);
  name[name_len] = '\0';
  offset += (data[offset - 1]); // Use actual length from packet

  // Parse Key
  if (offset >= len || data[offset++] != 0x73) {
    send_sw(SW_WRONG_DATA, apdu_out, len_out);
    return;
  }
  uint8_t key_block_len = data[offset++];

  if (offset + key_block_len > len) {
    send_sw(SW_WRONG_DATA, apdu_out, len_out);
    return;
  }

  uint8_t type_algo = data[offset++];
  uint8_t digits = data[offset++];

  oath_type_t type =
      (type_algo & 0xF0) == 0x10 ? OATH_TYPE_HOTP : OATH_TYPE_TOTP;
  oath_algo_t algo = (oath_algo_t)(type_algo & 0x0F);

  // Secret starts here
  uint8_t *secret = &data[offset];
  uint8_t secret_len = key_block_len - 2; // Subtract Type and Digits bytes
  offset += secret_len;

  // Check for Property Tag (0x78) - Custom Extension for Touch
  uint8_t touch_required = 0;
  if (offset < len && data[offset] == 0x78) {
    offset++;
    if (offset < len) {
      touch_required = data[offset++]; // 1 byte property
                                       // Mask 0x01 = Touch Required
      // In strict YK, properties might be inside 0x73, but we use extension tag
    }
  }

  if (oath_storage_put(name, secret, secret_len, type, algo, digits, 30,
                       touch_required)) {
    send_sw(SW_OK, apdu_out, len_out);
  } else {
    send_sw(SW_MEMORY_FAILURE, apdu_out, len_out);
  }
}

// Handler for DELETE Command (0x02)
static void handle_delete_command(uint8_t *data, uint16_t len,
                                  uint8_t *apdu_out, uint16_t *len_out) {
  // Access Control
  if (oath_storage_is_password_set() && !session_unlocked) {
    send_sw(SW_SECURITY_STATUS_NOT_SAT, apdu_out, len_out);
    return;
  }

  if (data[0] != 0x71) {
    send_sw(SW_WRONG_DATA, apdu_out, len_out);
    return;
  }
  uint8_t name_len = data[1];

  char name[OATH_MAX_NAME_LEN];
  if (name_len >= OATH_MAX_NAME_LEN)
    name_len = OATH_MAX_NAME_LEN - 1;
  memcpy(name, &data[2], name_len);
  name[name_len] = '\0';

  if (oath_storage_delete(name)) {
    send_sw(SW_OK, apdu_out, len_out);
  } else {
    send_sw(SW_FILE_NOT_FOUND, apdu_out, len_out);
  }
}

// Handler for LIST Command (0x00 or 0xA1 with no data?)
// Usually OATH uses 0xA1 (PUT/DEL/CALC/LIST share INS, distinguish by
// P1/P2/Data?) Actually Yubico spec says INS=0xA1. LIST uses tag 0x00? No,
// that's management. LIST is often "Get All" logic. Simpler implementation:
// Just return all names formatted.
static void handle_list_command(uint8_t *apdu_out, uint16_t *len_out) {
  uint16_t out_offset = 0;

  for (int i = 0; i < MAX_CREDENTIALS; i++) {
    const char *name = oath_storage_list(i);
    if (name) {
      // Tag 0x71, Len, Name
      uint8_t name_len = strlen(name);
      apdu_out[out_offset++] = 0x71;
      apdu_out[out_offset++] = name_len;
      memcpy(&apdu_out[out_offset], name, name_len);
      out_offset += name_len;

      // Tag 0x72 (Algo/Type) - Optional but useful
      oath_credential_t cred;
      if (oath_storage_get(name, &cred)) {
        apdu_out[out_offset++] = 0x72; // Proprietary tag? Or use standard
        apdu_out[out_offset++] = 1;
        apdu_out[out_offset++] = (uint8_t)cred.type | (uint8_t)cred.algorithm;
      }
    }
  }

  send_sw(SW_OK, apdu_out + out_offset, len_out);
  *len_out += out_offset;
}

// Handler for CALCULATE Command (0x04 for TRUNCATED Response)
// Data: [0x71, name_len, name, 0x74, 8, challenge...]
static void handle_calculate_command(uint8_t *data, uint16_t len,
                                     uint8_t *apdu_out, uint16_t *len_out) {
  // Parse Name
  uint16_t offset = 0;
  if (data[offset++] != 0x71) {
    send_sw(SW_WRONG_DATA, apdu_out, len_out);
    return;
  }
  uint8_t name_len = data[offset++];

  char name[OATH_MAX_NAME_LEN];
  if (name_len >= OATH_MAX_NAME_LEN)
    name_len = OATH_MAX_NAME_LEN - 1;
  memcpy(name, &data[offset], name_len);
  name[name_len] = '\0';
  offset += name_len;

  // Parse Challenge (for HOTP/TOTP often time is passed or implicit)
  // If Challenge present (Tag 0x74), use it. Else use internal time (for TOTP).
  int64_t timestamp = 0; // Or counter
  bool has_challenge = false;

  if (offset < len && data[offset] == 0x74) {
    offset++;
    uint8_t chall_len = data[offset++];
    if (chall_len == 8) {
      // 8 byte challenge (counter or time)
      // Big endian?
      for (int i = 0; i < 8; i++) {
        timestamp = (timestamp << 8) | data[offset++];
      }
      has_challenge = true;
    }
  }

  // Load Credential
  oath_credential_t cred;
  if (!oath_storage_get(name, &cred)) {
    send_sw(SW_FILE_NOT_FOUND, apdu_out, len_out);
    return;
  }

  // Check Touch Requirement
  if (cred.touch_required) {
    if (!do_touch_check()) {
      send_sw(SW_CONDITIONS_NOT_SATISFIED, apdu_out, len_out);
      return;
    }
  }

  // Prepare Secret (Convert binary to Base32 for libcotp)
  cotp_error_t err;
  char *base32_secret = base32_encode(cred.secret, cred.secret_len, &err);
  if (!base32_secret) {
    send_sw(SW_UNKNOWN, apdu_out, len_out);
    return;
  }

  char *otp_string = NULL;

  if (cred.type == OATH_TYPE_TOTP) {
    // If no challenge provided, use system time
    if (!has_challenge) {
      // Try to get time from time sync
      timestamp = time_sync_get_timestamp();
      if (timestamp == 0) {
        // Fallback to relative time
        static uint64_t fallback_time = 1000000000;
        timestamp = fallback_time++;
      }
    }

    // Map Algo
    int sha_algo;
    switch (cred.algorithm) {
    case OATH_ALGO_SHA1:
      sha_algo = SHA1;
      break;
    case OATH_ALGO_SHA256:
      sha_algo = SHA256;
      break;
    case OATH_ALGO_SHA512:
      sha_algo = SHA512;
      break;
    default:
      sha_algo = SHA1;
    }

    otp_string = get_totp_at(base32_secret, (long)timestamp, cred.digits,
                             cred.period, sha_algo, &err);

  } else { // HOTP
    int sha_algo =
        (cred.algorithm == OATH_ALGO_SHA256) ? SHA256 : SHA1; // Simplification

    long counter = cred.counter;
    if (has_challenge) {
      // For HOTP, Challenge can override counter
      counter = (long)timestamp;
    }

    otp_string = get_hotp(base32_secret, counter, cred.digits, sha_algo, &err);

    // Update counter
    oath_storage_update_counter(name, counter + 1);
  }

  free(base32_secret);

  if (otp_string) {
    // Response: Tag 0x76 (Response), Len, Digits (Text or Int?)
    int otp_len = strlen(otp_string);
    apdu_out[0] = 0x76;
    apdu_out[1] = otp_len;
    memcpy(&apdu_out[2], otp_string, otp_len);

    send_sw(SW_OK, apdu_out + 2 + otp_len, len_out);
    *len_out += (2 + otp_len);

    free(otp_string);
  } else {
    send_sw(SW_UNKNOWN, apdu_out, len_out);
  }
}

// Handler for SET CODE (0x03)
static void handle_set_code(uint8_t *data, uint16_t len, uint8_t *apdu_out,
                            uint16_t *len_out) {
  // Format: Key + Challenge (New Password)
  // Actually Yk: 0x03 | Key + 8 byte challenge (derivation/salt?)
  // Simplification: Data IS the new password
  if (oath_storage_set_password(data, (uint8_t)len)) {
    session_unlocked = true; // Auto unlock after setting?
    send_sw(SW_OK, apdu_out, len_out);
  } else {
    send_sw(SW_MEMORY_FAILURE, apdu_out, len_out);
  }
}

// Handler for TIME SYNC (Custom command 0x05)
static void handle_time_sync(uint8_t *data, uint16_t len, uint8_t *apdu_out,
                             uint16_t *len_out) {
  // Data: 8 bytes Unix timestamp (big-endian)
  if (len < 8) {
    send_sw(SW_WRONG_LENGTH, apdu_out, len_out);
    return;
  }

  uint64_t timestamp = 0;
  for (int i = 0; i < 8; i++) {
    timestamp = (timestamp << 8) | data[i];
  }

  time_sync_set_timestamp(timestamp);

  // Response: 1 byte status (0x01 = success)
  apdu_out[0] = 0x01;
  *len_out = 1;
  send_sw(SW_OK, apdu_out + 1, len_out);
  *len_out += 1;
}

// Handler for VALIDATE (0xA3)
static void handle_validate(uint8_t *data, uint16_t len, uint8_t *apdu_out,
                            uint16_t *len_out) {
  // Data: Password/Challenge response
  if (oath_storage_verify_password(data, (uint8_t)len)) {
    session_unlocked = true;
    send_sw(SW_OK, apdu_out, len_out);
  } else {
    session_unlocked = false;
    send_sw(SW_SECURITY_STATUS_NOT_SAT, apdu_out, len_out);
  }
}

// Handler for GET VERSION (0x06)
static void handle_version_command(uint8_t *apdu_out, uint16_t *len_out) {
  // Return version 5.4.3 (consistent with a modern YubiKey 5)
  apdu_out[0] = 0x05;
  apdu_out[1] = 0x04;
  apdu_out[2] = 0x03;
  *len_out = 3;
  send_sw(SW_OK, apdu_out + 3, len_out);
  *len_out += 3;
}

// Handler for the Yubico OATH CALCULATE/LIST command (INS=0xA1 or others)
static void handle_oath_command(uint8_t *apdu_in, uint16_t len_in,
                                uint8_t *apdu_out, uint16_t *len_out) {

  // Tag is first byte of Data
  uint8_t instr = apdu_in[APDU_INS_POS];

  if (instr == 0x01) { // PUT
    handle_put_command(apdu_in + APDU_DATA_POS, apdu_in[APDU_LC_POS], apdu_out,
                       len_out);
  } else if (instr == 0x02) { // DELETE
    handle_delete_command(apdu_in + APDU_DATA_POS, apdu_in[APDU_LC_POS],
                          apdu_out, len_out);
  } else if (instr == 0x03) { // SET CODE
    handle_set_code(apdu_in + APDU_DATA_POS, apdu_in[APDU_LC_POS], apdu_out,
                    len_out);
  } else if (instr == 0xA3) { // VALIDATE
    handle_validate(apdu_in + APDU_DATA_POS, apdu_in[APDU_LC_POS], apdu_out,
                    len_out);

  } else if (instr ==
             0x04) { // RESET or CALCULATE (Wait, Yubico uses INS? or Tags?)
    // Yubico: INS=0x01 PUT, INS=0x02 DELETE, INS=0x03 SET CODE, INS=0x04 RESET
    // INS=0x00? LIST?
    // Actually common OATH over CCID uses INS=0xA1 for management often, or
    // standard ISO. Let's map strict instruction codes as per implementation
    // plan or standard. Assuming Implementation Plan's mapping: INS=0x01 PUT
    // INS=0x02 DELETE
    // INS=0x04 RESET/Wipe
    // INS=0xA1 (CALCULATE / LIST dispatch based on tags)

    // Let's stick to INS being the differentiator if distinct INS are used.
    oath_storage_reset();
    send_sw(SW_OK, apdu_out, len_out);
  } else if (instr == 0xA1) { // LIST or CALCULATE (Shared INS)
    // Check Tag in Data
    if (len_in <= 5) { // No data, might be LIST
      handle_list_command(apdu_out, len_out);
      return;
    }

    uint8_t tag = apdu_in[APDU_DATA_POS];
    if (tag == 0x74) { // CALCULATE
      handle_calculate_command(apdu_in + APDU_DATA_POS, apdu_in[APDU_LC_POS],
                               apdu_out, len_out);
    } else if (tag == 0x71) { // INPUT Name for CALCULATE?
      handle_calculate_command(apdu_in + APDU_DATA_POS, apdu_in[APDU_LC_POS],
                               apdu_out, len_out);
    } else {
      // Default to List?
      handle_list_command(apdu_out, len_out);
    }
  } else {
    send_sw(SW_INS_NOT_SUPPORTED, apdu_out, len_out);
  }
}

// Placeholder for OATH protocol initialization
void oath_init(void) {
  printf("OATH: Initializing protocol handler...\n");
  led_driver_init();      // Init WS2812
  led_set_color(0, 0, 0); // Off initially
  oath_storage_init();
  session_unlocked = false;
  gpio_init(TOUCH_BUTTON_PIN);
  gpio_set_dir(TOUCH_BUTTON_PIN, GPIO_IN);
  gpio_pull_up(TOUCH_BUTTON_PIN); // Assume Active Low button
}

// Main APDU handling function (called from CCID driver)
void oath_handle_apdu(uint8_t *apdu_in, uint16_t len_in, uint8_t *apdu_out,
                      uint16_t *len_out) {
  // APDU structure: CLA | INS | P1 | P2 | Lc | Data | Le

  if (len_in < 5) {
    send_sw(SW_WRONG_LENGTH, apdu_out, len_out);
    return;
  }

  uint8_t cla = apdu_in[APDU_CLA_POS];
  uint8_t ins = apdu_in[APDU_INS_POS];

  if (cla != 0x00) {
    send_sw(SW_CLA_NOT_SUPPORTED, apdu_out, len_out);
    return;
  }

  switch (ins) {
  case INS_SELECT:
    handle_select_apdu(apdu_in, len_in, apdu_out, len_out);
    break;

  case 0x01: // PUT
  case 0x02: // DELETE
  case 0x03: // SET CODE
  case 0x04: // RESET or TIME SYNC (custom)
  case 0x06: // VERSION
  case 0xA1: // CALC or LIST
  case 0xA3: // VALIDATE
    if (ins == 0x06) {
      handle_version_command(apdu_out, len_out);
    } else {
      handle_oath_command(apdu_in, len_in, apdu_out, len_out);
    }
    break;

  case 0x05: // TIME SYNC (custom command)
    handle_time_sync(apdu_in + APDU_DATA_POS, apdu_in[APDU_LC_POS], apdu_out,
                     len_out);
    break;

  default:
    send_sw(SW_INS_NOT_SUPPORTED, apdu_out, len_out);
    break;
  }
}
