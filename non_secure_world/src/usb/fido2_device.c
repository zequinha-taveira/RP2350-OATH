#include "fido2_device.h"
#include "pico/rand.h"
#include "pico/stdlib.h"
#include "secure_gateway.h"
#include "tusb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//--------------------------------------------------------------------+
// FIDO2 Device State
//--------------------------------------------------------------------+

static fido2_state_t fido2_state = {.initialized = false,
                                    .connected = false,
                                    .current_channel = 0,
                                    .nonce = {0},
                                    .pin_token = {0},
                                    .pin_protocol = 1,
                                    .max_msg_size = 1024};

// HID Report Buffer
static uint8_t hid_report_in[FIDO2_REPORT_SIZE];
static uint8_t hid_report_out[FIDO2_REPORT_SIZE];

// Channel ID management
static uint32_t next_channel_id = FIDO2_CHANNEL_FIRST;

// HID Report Descriptor for FIDO2
// 52 bytes as expected by usb_descriptors.c (0x34)
uint8_t const desc_hid_report[] = {
    0x06, 0xD0, 0xF1, // Usage Page (FIDO Alliance)
    0x09, 0x01,       // Usage (U2F Authenticator Device)
    0xA1, 0x01,       // Collection (Application)
    0x09, 0x20,       //   Usage (Data In)
    0x15, 0x00,       //   Logical Minimum (0)
    0x26, 0xFF, 0x00, //   Logical Maximum (255)
    0x75, 0x08,       //   Report Size (8)
    0x95, 0x40,       //   Report Count (64)
    0x81, 0x02,       //   Input (Data, Absolute, Variable)
    0x09, 0x21,       //   Usage (Data Out)
    0x15, 0x00,       //   Logical Minimum (0)
    0x26, 0xFF, 0x00, //   Logical Maximum (255)
    0x75, 0x08,       //   Report Size (8)
    0x95, 0x40,       //   Report Count (64)
    0x91, 0x02,       //   Output (Data, Absolute, Variable)
    0xC0              // End Collection
};

// Forward declarations
void fido2_process_frame(ctaphid_frame_t const *frame);
void fido2_send_response(uint8_t cmd, uint8_t const *data, uint16_t len);
void fido2_send_init_response(uint8_t const *nonce);
void fido2_handle_init(ctaphid_frame_t const *frame);
void fido2_handle_ping(ctaphid_frame_t const *frame);
void fido2_handle_msg(ctaphid_frame_t const *frame);
void fido2_handle_cancel(void);
void fido2_handle_wink(void);
void fido2_handle_make_credential(uint8_t const *data, uint16_t len);
void fido2_handle_get_assertion(uint8_t const *data, uint16_t len);
void fido2_handle_get_info(void);
void fido2_handle_client_pin(uint8_t const *data, uint16_t len);
void fido2_handle_reset(void);

// CBOR encoding helpers
uint8_t *cbor_encode_map(uint8_t *buffer, uint8_t size) {
  if (size <= 23) {
    *buffer++ = 0xA0 + size;
  } else if (size <= 255) {
    *buffer++ = 0xB8;
    *buffer++ = size;
  }
  return buffer;
}

uint8_t *cbor_encode_uint(uint8_t *buffer, uint64_t value) {
  if (value <= 23) {
    *buffer++ = (uint8_t)value;
  } else if (value <= 255) {
    *buffer++ = 0x18;
    *buffer++ = (uint8_t)value;
  } else if (value <= 65535) {
    *buffer++ = 0x19;
    *buffer++ = (uint8_t)(value >> 8);
    *buffer++ = (uint8_t)value;
  } else {
    *buffer++ = 0x1A;
    *buffer++ = (uint8_t)(value >> 24);
    *buffer++ = (uint8_t)(value >> 16);
    *buffer++ = (uint8_t)(value >> 8);
    *buffer++ = (uint8_t)value;
  }
  return buffer;
}

uint8_t *cbor_encode_string(uint8_t *buffer, const char *str) {
  uint8_t len = strlen(str);
  if (len <= 23) {
    *buffer++ = 0x60 + len;
  } else if (len <= 255) {
    *buffer++ = 0x78;
    *buffer++ = len;
  }
  memcpy(buffer, str, len);
  return buffer + len;
}

uint8_t *cbor_encode_bytes(uint8_t *buffer, const uint8_t *data, uint16_t len) {
  if (len <= 23) {
    *buffer++ = 0x40 + len;
  } else if (len <= 255) {
    *buffer++ = 0x58;
    *buffer++ = len;
  } else {
    *buffer++ = 0x59;
    *buffer++ = (uint8_t)(len >> 8);
    *buffer++ = (uint8_t)len;
  }
  memcpy(buffer, data, len);
  return buffer + len;
}

uint8_t *cbor_encode_array(uint8_t *buffer, uint8_t size) {
  if (size <= 23) {
    *buffer++ = 0x80 + size;
  } else if (size <= 255) {
    *buffer++ = 0x98;
    *buffer++ = size;
  }
  return buffer;
}

void fido2_init(void) {
  printf("FIDO2: Initializing\n");
  fido2_state.initialized = true;
  fido2_state.connected = false;
  fido2_state.current_channel = 0;

  // Generate random nonce for init
  for (int i = 0; i < 8; i++) {
    fido2_state.nonce[i] = (uint8_t)rand();
  }

  printf("FIDO2: Initialized with nonce\n");
}

void fido2_task(void) {
  // FIDO2 task handler (if needed)
}

//--------------------------------------------------------------------+
// TinyUSB HID Callbacks
//--------------------------------------------------------------------+

// Invoked when received GET HID REPORT DESCRIPTOR
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
  (void)instance;
  return desc_hid_report;
}

// Invoked when received GET_REPORT control request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id,
                               hid_report_type_t report_type, uint8_t *buffer,
                               uint16_t reqlen) {
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;
  return 0;
}

// Invoked when received SET_REPORT control request or received data on OUT
// endpoint
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id,
                           hid_report_type_t report_type, uint8_t const *buffer,
                           uint16_t bufsize) {
  (void)instance;
  (void)report_id;
  (void)report_type;
  fido2_handle_report(buffer, bufsize);
}

//--------------------------------------------------------------------+
// Custom FIDO2 Class Driver
//--------------------------------------------------------------------+

static void fido2_user_init(void) { fido2_init(); }
static void fido2_user_reset(uint8_t rhport) { (void)rhport; }
static uint16_t fido2_user_open(uint8_t rhport,
                                tusb_desc_interface_t const *desc_intf,
                                uint16_t max_len) {
  // FIDO2 is handled by standard HID class in this implementation
  // This custom driver placeholder is for more advanced implementations
  return 0;
}
static bool fido2_user_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                       tusb_control_request_t const *request) {
  return false;
}
static bool fido2_user_xfer_cb(uint8_t rhport, uint8_t ep_addr,
                               xfer_result_t result, uint32_t xferred_bytes) {
  return false;
}

const usbd_class_driver_t fido2_driver = {.name = "FIDO2",
                                          .init = fido2_user_init,
                                          .reset = fido2_user_reset,
                                          .open = fido2_user_open,
                                          .control_xfer_cb =
                                              fido2_user_control_xfer_cb,
                                          .xfer_cb = fido2_user_xfer_cb,
                                          .sof = NULL};

void fido2_handle_report(uint8_t const *report, uint32_t len) {
  if (len < 3)
    return; // Minimum frame size

  ctaphid_frame_t const *frame = (ctaphid_frame_t const *)report;

  printf("FIDO2: RX Cmd=0x%02X, Len=%u\n", frame->cmd,
         (frame->bcnth << 8) | frame->bcntl);

  fido2_process_frame(frame);
}

void fido2_process_frame(ctaphid_frame_t const *frame) {
  uint16_t data_len = (frame->bcnth << 8) | frame->bcntl;

  switch (frame->cmd) {
  case CTAPHID_INIT:
    fido2_handle_init(frame);
    break;

  case CTAPHID_PING:
    fido2_handle_ping(frame);
    break;

  case CTAPHID_MSG:
    fido2_handle_msg(frame);
    break;

  case CTAPHID_CANCEL:
    fido2_handle_cancel();
    break;

  case CTAPHID_WINK:
    fido2_handle_wink();
    break;

  case CTAPHID_LOCK:
    // Not implemented for this simple version
    fido2_send_error(CTAP1_ERR_INVALID_COMMAND);
    break;

  default:
    printf("FIDO2: Unknown command 0x%02X\n", frame->cmd);
    fido2_send_error(CTAP1_ERR_INVALID_COMMAND);
    break;
  }
}

void fido2_handle_init(ctaphid_frame_t const *frame) {
  printf("FIDO2: INIT command received\n");

  // Verify nonce
  if (memcmp(frame->data, fido2_state.nonce, 8) != 0) {
    printf("FIDO2: INIT nonce mismatch\n");
    return;
  }

  // Generate new channel ID
  uint32_t new_channel = next_channel_id++;
  if (next_channel_id > FIDO2_CHANNEL_LAST) {
    next_channel_id = FIDO2_CHANNEL_FIRST;
  }

  // Build init response
  ctaphid_init_response_t response = {.cmd = CTAPHID_INIT,
                                      .bcnth = 0,
                                      .bcntl = 17,
                                      .nonce = {0},
                                      .cid = {0},
                                      .version =
                                          0x02, // CTAPHID protocol version
                                      .capabilities = 0x01, // Supports WINK
                                      .reserved = {0}};

  // Copy nonce from request
  memcpy(response.nonce, frame->data, 8);

  // Set new channel ID
  response.cid[0] = (uint8_t)(new_channel >> 24);
  response.cid[1] = (uint8_t)(new_channel >> 16);
  response.cid[2] = (uint8_t)(new_channel >> 8);
  response.cid[3] = (uint8_t)new_channel;

  fido2_state.current_channel = new_channel;
  fido2_state.connected = true;

  printf("FIDO2: INIT response, new channel: 0x%08X\n", new_channel);

  fido2_send_report((uint8_t const *)&response, sizeof(response));
}

void fido2_handle_ping(ctaphid_frame_t const *frame) {
  uint16_t data_len = (frame->bcnth << 8) | frame->bcntl;
  printf("FIDO2: PING command, %u bytes\n", data_len);

  // Echo back the data
  uint8_t response[64];
  response[0] = CTAPHID_PING;
  response[1] = frame->bcnth;
  response[2] = frame->bcntl;
  memcpy(response + 3, frame->data, data_len);

  fido2_send_report(response, 3 + data_len);
}

void fido2_handle_msg(ctaphid_frame_t const *frame) {
  uint16_t data_len = (frame->bcnth << 8) | frame->bcntl;
  printf("FIDO2: MSG command, %u bytes\n", data_len);

  if (data_len < 1) {
    fido2_send_error(CTAP1_ERR_INVALID_LENGTH);
    return;
  }

  // First byte is CTAP2 command
  uint8_t ctap_cmd = frame->data[0];
  uint8_t const *ctap_data = frame->data + 1;
  uint16_t ctap_len = data_len - 1;

  printf("FIDO2: CTAP2 command 0x%02X\n", ctap_cmd);

  // Send processing keepalive
  fido2_send_keepalive(CTAP2_KEEPALIVE_STATUS_PROCESSING);

  uint8_t response[1024];
  uint16_t response_len = 0;

  if (secure_gateway_fido2_handle_msg(frame->data, data_len, response,
                                      &response_len)) {
    // Send as CTAPHID_MSG
    // For simplicity, we assume single-packet response for now
    // In a real implementation, we'd handle fragments
    uint8_t hid_response[64];
    memset(hid_response, 0, 64);
    hid_response[0] = CTAPHID_MSG;
    hid_response[1] = (response_len >> 8) & 0xFF;
    hid_response[2] = response_len & 0xFF;

    // Copy first chunk
    uint16_t chunk_len = (response_len > 61) ? 61 : response_len;
    memcpy(hid_response + 3, response, chunk_len);

    fido2_send_report(hid_response, 64);

    // TODO: Handle fragments if response_len > 61
  } else {
    fido2_send_error(CTAP2_ERR_OTHER);
  }
}

void fido2_handle_cancel(void) {
  printf("FIDO2: CANCEL command\n");
  // Cancel any pending operation
  fido2_send_error(CTAP1_ERR_SUCCESS);
}

void fido2_handle_wink(void) {
  printf("FIDO2: WINK command\n");
  // Wink response (visual feedback)
  uint8_t response[3];
  response[0] = CTAPHID_WINK;
  response[1] = 0;
  response[2] = 0;
  fido2_send_report(response, 3);
}

ptr = cbor_encode_uint(ptr, 0x05);
ptr = cbor_encode_uint(ptr, fido2_state.max_msg_size);

// Pin protocols
ptr = cbor_encode_uint(ptr, 0x06);
ptr = cbor_encode_array(ptr, 1);
ptr = cbor_encode_uint(ptr, fido2_state.pin_protocol);

uint16_t response_len = ptr - response;

// Send as CTAPHID_MSG
uint8_t hid_response[64];
hid_response[0] = CTAPHID_MSG;
hid_response[1] = (response_len >> 8) & 0xFF;
hid_response[2] = response_len & 0xFF;
memcpy(hid_response + 3, response, response_len);

fido2_send_report(hid_response, 3 + response_len);
}

void fido2_handle_client_pin(uint8_t const *data, uint16_t len) {
  printf("FIDO2: CLIENT_PIN\n");

  // Simplified PIN handling
  // In real implementation, this would handle PIN operations

  uint8_t response[16];
  uint8_t *ptr = response;

  // CTAP2 command response
  *ptr++ = CTAP2_CLIENT_PIN;

  // CBOR map
  ptr = cbor_encode_map(ptr, 1);
  ptr = cbor_encode_uint(ptr, 0x01); // Pin token
  uint8_t token[32] = {0};
  ptr = cbor_encode_bytes(ptr, token, 32);

  uint16_t response_len = ptr - response;

  // Send as CTAPHID_MSG
  uint8_t hid_response[64];
  hid_response[0] = CTAPHID_MSG;
  hid_response[1] = (response_len >> 8) & 0xFF;
  hid_response[2] = response_len & 0xFF;
  memcpy(hid_response + 3, response, response_len);

  fido2_send_report(hid_response, 3 + response_len);
}

void fido2_handle_reset(void) {
  printf("FIDO2: RESET\n");

  // Reset all credentials
  // In real implementation, this would clear storage

  uint8_t response[3];
  response[0] = CTAP2_RESET;
  response[1] = 0;
  response[2] = 0;

  fido2_send_report(response, 3);
}

void fido2_send_keepalive(uint8_t status) {
  uint8_t report[64];
  report[0] = CTAPHID_KEEPALIVE;
  report[1] = 0;
  report[2] = 1;
  report[3] = status;

  fido2_send_report(report, 4);
}

void fido2_send_error(uint8_t error_code) {
  uint8_t report[64];
  report[0] = CTAPHID_ERROR;
  report[1] = 0;
  report[2] = 1;
  report[3] = error_code;

  fido2_send_report(report, 4);
}

void fido2_send_response(uint8_t cmd, uint8_t const *data, uint16_t len) {
  uint8_t report[64];
  report[0] = cmd;
  report[1] = (len >> 8) & 0xFF;
  report[2] = len & 0xFF;
  memcpy(report + 3, data, len);

  fido2_send_report(report, 3 + len);
}

bool fido2_send_report(uint8_t const *report, uint32_t len) {
  if (!fido2_state.connected || len > FIDO2_REPORT_SIZE)
    return false;

  // Use TinyUSB HID API
  return tud_hid_report(0, report, len);
}

// User verification (simplified - would require button press)
bool fido2_verify_user(void) {
  printf("FIDO2: User verification requested\n");
  // In real implementation, this would wait for button press
  // For now, return true after a short delay
  sleep_ms(100);
  return true;
}

// PIN verification (simplified)
bool fido2_verify_pin(uint8_t *pin, uint8_t pin_len) {
  printf("FIDO2: PIN verification\n");
  // In real implementation, verify against stored hash
  return true;
}

// PIN setup (simplified)
bool fido2_set_pin(uint8_t *pin, uint8_t pin_len) {
  printf("FIDO2: PIN setup\n");
  // In real implementation, store PIN hash
  return true;
}

// Credential storage (simplified - would use secure storage)
bool fido2_store_credential(uint8_t *credential_id, uint8_t *credential_data,
                            uint16_t data_len) {
  printf("FIDO2: Storing credential\n");
  // In real implementation, encrypt and store in flash
  return true;
}

bool fido2_retrieve_credential(uint8_t *credential_id, uint8_t *credential_data,
                               uint16_t *data_len) {
  printf("FIDO2: Retrieving credential\n");
  // In real implementation, decrypt from flash
  return true;
}

bool fido2_delete_credential(uint8_t *credential_id) {
  printf("FIDO2: Deleting credential\n");
  // In real implementation, remove from flash
  return true;
}