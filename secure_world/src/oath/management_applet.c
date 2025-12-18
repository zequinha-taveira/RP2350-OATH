#include "management_applet.h"
#include "apdu_protocol.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/**
 * @file management_applet.c
 * @brief Yubico-compatible Management Applet implementation.
 */

// Simulation of device configuration (First Boot parameters)
typedef struct {
  uint16_t vid;
  uint16_t pid;
  uint8_t version[3];
  uint32_t serial;
  uint8_t secure_boot_enabled;
  uint8_t secure_lock_enabled;
} device_config_t;

static device_config_t global_config = {
    .vid = 0x1050,
    .pid = 0x0407,
    .version = {5, 4, 3},
    .serial = 12345678,
    .secure_boot_enabled = 1,
    .secure_lock_enabled = 1 // Configuration is locked by default for security
};

static void send_sw(uint16_t sw, uint8_t *apdu_out, uint16_t *len_out) {
  apdu_out[0] = (uint8_t)(sw >> 8);
  apdu_out[1] = (uint8_t)(sw & 0xFF);
  *len_out = 2;
}

void management_applet_init(void) {
  printf("[MGMT] Management Applet Initialized (Yubico Compatibility Mode)\n");
}

void management_applet_handle_apdu(uint8_t *apdu_in, uint16_t len_in,
                                   uint8_t *apdu_out, uint16_t *len_out) {
  if (len_in < 4) {
    send_sw(SW_WRONG_LENGTH, apdu_out, len_out);
    return;
  }

  uint8_t ins = apdu_in[APDU_INS_POS];

  if (ins == INS_SELECT) {
    // Management SELECT Response: TLV Data
    uint16_t offset = 0;

    // Tag 0x01: Supported USB Capabilities
    apdu_out[offset++] = 0x01;
    apdu_out[offset++] = 0x01;
    apdu_out[offset++] = 0x7F; // OTP | FIDO | CCID | OPGP | PIV | OATH

    // Tag 0x02: Serial Number
    apdu_out[offset++] = 0x02;
    apdu_out[offset++] = 0x04;
    apdu_out[offset++] = (uint8_t)(global_config.serial >> 24);
    apdu_out[offset++] = (uint8_t)(global_config.serial >> 16);
    apdu_out[offset++] = (uint8_t)(global_config.serial >> 8);
    apdu_out[offset++] = (uint8_t)(global_config.serial & 0xFF);

    // Tag 0x03: Enabled USB Capabilities
    apdu_out[offset++] = 0x03;
    apdu_out[offset++] = 0x01;
    apdu_out[offset++] = 0x7F;

    // Tag 0x04: Form Factor
    apdu_out[offset++] = 0x04;
    apdu_out[offset++] = 0x01;
    apdu_out[offset++] = 0x01; // USB-A

    // Tag 0x05: Firmware Version
    apdu_out[offset++] = 0x05;
    apdu_out[offset++] = 0x03;
    apdu_out[offset++] = global_config.version[0];
    apdu_out[offset++] = global_config.version[1];
    apdu_out[offset++] = global_config.version[2];

    // Tag 0x08: Device Flags (0x00)
    apdu_out[offset++] = 0x08;
    apdu_out[offset++] = 0x01;
    apdu_out[offset++] = 0x00;

    // Tag 0x0D: Supported NFC Capabilities
    apdu_out[offset++] = 0x0D;
    apdu_out[offset++] = 0x01;
    apdu_out[offset++] = 0x7F;

    // Tag 0x0E: Enabled NFC Capabilities
    apdu_out[offset++] = 0x0E;
    apdu_out[offset++] = 0x01;
    apdu_out[offset++] = 0x7F;

    // Tag 0x0A: Configuration Lock (0x01 if locked)
    apdu_out[offset++] = 0x0A;
    apdu_out[offset++] = 0x01;
    apdu_out[offset++] = global_config.secure_lock_enabled;

    send_sw(SW_OK, apdu_out + offset, len_out);
    *len_out = offset + 2;
    return;
  }

  // INS 0x1F: GET DEVICE INFO (Professional detailed info)
  if (ins == 0x1F) {
    uint16_t offset = 0;
    // Serial (Tag 0x02)
    apdu_out[offset++] = 0x02;
    apdu_out[offset++] = 0x04;
    apdu_out[offset++] = (uint8_t)(global_config.serial >> 24);
    apdu_out[offset++] = (uint8_t)(global_config.serial >> 16);
    apdu_out[offset++] = (uint8_t)(global_config.serial >> 8);
    apdu_out[offset++] = (uint8_t)(global_config.serial & 0xFF);

    // Device Flags (Tag 0x08)
    apdu_out[offset++] = 0x08;
    apdu_out[offset++] = 0x01;
    apdu_out[offset++] = global_config.secure_boot_enabled ? 0x01 : 0x00;

    send_sw(SW_OK, apdu_out + offset, len_out);
    *len_out = offset + 2;
    return;
  }

  // INS 0x1D: GET FLASH CONFIG (Pseudo-command for demo/init)
  if (ins == 0x1D) {
    memcpy(apdu_out, &global_config, sizeof(device_config_t));
    *len_out = sizeof(device_config_t);
    send_sw(SW_OK, apdu_out + *len_out, len_out);
    *len_out += 2;
    return;
  }

  send_sw(SW_INS_NOT_SUPPORTED, apdu_out, len_out);
}
