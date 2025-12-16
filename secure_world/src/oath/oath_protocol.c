#include "oath_protocol.h"
#include "apdu_protocol.h" // For APDU and OATH constants
#include "cotp.h"          // Using libcotp for core logic
#include "oath_storage.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global state for the OATH application
static bool oath_app_selected = false;
static uint32_t last_list_index = 0; // For paged listing (not strictly required
                                     // by Yubico spec but good for large lists)

// Helper function to send a status word (SW) response
static void send_sw(uint16_t sw, uint8_t *apdu_out, uint16_t *len_out) {
  apdu_out[0] = (uint8_t)(sw >> 8);
  apdu_out[1] = (uint8_t)(sw & 0xFF);
  *len_out = 2;
}

// Handler for the ISO 7816 SELECT command (INS=A4)
static void handle_select_apdu(uint8_t *apdu_in, uint16_t len_in,
                               uint8_t *apdu_out, uint16_t *len_out) {
  if (apdu_in[APDU_P1_POS] != 0x04 || apdu_in[APDU_P2_POS] != 0x00) {
    send_sw(SW_WRONG_LENGTH, apdu_out, len_out);
    return;
  }

  uint8_t lc = apdu_in[APDU_LC_POS];
  uint8_t *aid = apdu_in + APDU_DATA_POS;

  if (lc == OATH_AID_LEN && memcmp(aid, OATH_AID, OATH_AID_LEN) == 0) {
    oath_app_selected = true;
    printf("OATH: Application selected successfully.\n");
    // Standard response for OATH SELECT is empty or version info + SW_OK
    // For simplicity, just SW_OK
    send_sw(SW_OK, apdu_out, len_out);
  } else {
    oath_app_selected = false;
    printf("OATH: Application selection failed.\n");
    send_sw(SW_FILE_NOT_FOUND, apdu_out, len_out);
  }
}

// Handler for PUT Command (0x01)
// Data: [0x71, name_len, name..., 0x73, key_len (including type byte + 4 bytes
// properties), type (0x1x HOTP, 0x2x TOTP), digits, algo, secret...]
static void handle_put_command(uint8_t *data, uint16_t len, uint8_t *apdu_out,
                               uint16_t *len_out) {
  // Basic TLV parsing
  // Structure expected:
  // Name Tag (0x71) | Len | Name
  // Key Tag (0x73)  | Len | Type | Digits | Period/Counter... | Secret

  // Simplification: We assume strictly formatted Input
  // Example: 71 04 "Test" 73 16 21 06 00... (secret)

  uint16_t offset = 0;

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

  // Key Block Structure: Type(1) + Digits(1) + Properties(4?) + Secret(rest)
  // Actually standard is:
  // Type/Algo byte: 0x21 (TOTP+SHA1), 0x22 (TOTP+SHA256)
  // Digits: 6, 7, 8
  // Secret

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

  if (oath_storage_put(name, secret, secret_len, type, algo, digits, 30)) {
    send_sw(SW_OK, apdu_out, len_out);
  } else {
    send_sw(SW_MEMORY_FAILURE, apdu_out, len_out);
  }
}

// Handler for DELETE Command (0x02)
// Data: [0x71, name_len, name...]
static void handle_delete_command(uint8_t *data, uint16_t len,
                                  uint8_t *apdu_out, uint16_t *len_out) {
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
  offset += (data[offset - 1]);

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

  // Prepare Secret (Convert binary to Base32 for libcotp)
  cotp_error_t err;
  char *base32_secret = base32_encode(cred.secret, cred.secret_len, &err);
  if (!base32_secret) {
    send_sw(SW_UNKNOWN, apdu_out, len_out);
    return;
  }

  char *otp_string = NULL;

  if (cred.type == OATH_TYPE_TOTP) {
    // If no challenge provided, use system time (placeholder)
    if (!has_challenge) {
      // TODO: Get real time. For now hardcode.
      timestamp = 1000000000;
    } else {
      // For TOTP, challenge is usually time in seconds.
      // libcotp expects timestamp.
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
    if (has_challenge)
      counter = (long)timestamp; // Override

    otp_string = get_hotp(base32_secret, counter, cred.digits, sha_algo, &err);

    // Update counter
    oath_storage_update_counter(name, counter + 1);
  }

  free(base32_secret);

  if (otp_string) {
    // Response: Tag 0x76 (Response), Len, Digits (Text or Int?)
    // Usually 0x76 is truncation result (4 bytes) or full string?
    // Standard OATH response: 0x76 | Len | b1 b2 b3 b4 (Int) or String?
    // Let's send 4 byte integer for compatibility handling, or digits string?

    // Yubico returns TLV: 76 05 (Digits)
    // If string needed:
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

// Handler for the Yubico OATH CALCULATE/LIST command (INS=0xA1 or others)
static void handle_oath_command(uint8_t *apdu_in, uint16_t len_in,
                                uint8_t *apdu_out, uint16_t *len_out) {
  if (!oath_app_selected) {
    send_sw(SW_SECURITY_STATUS_NOT_SAT, apdu_out, len_out);
    return;
  }

  // Tag is first byte of Data
  uint8_t instr = apdu_in[APDU_INS_POS];

  if (instr == 0x01) { // PUT
    handle_put_command(apdu_in + APDU_DATA_POS, apdu_in[APDU_LC_POS], apdu_out,
                       len_out);
  } else if (instr == 0x02) { // DELETE
    handle_delete_command(apdu_in + APDU_DATA_POS, apdu_in[APDU_LC_POS],
                          apdu_out, len_out);
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
  oath_storage_init();
  oath_app_selected = false;
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
  case 0x04: // RESET
  case 0xA1: // CALC or LIST
    handle_oath_command(apdu_in, len_in, apdu_out, len_out);
    break;

  default:
    send_sw(SW_INS_NOT_SUPPORTED, apdu_out, len_out);
    break;
  }
}
