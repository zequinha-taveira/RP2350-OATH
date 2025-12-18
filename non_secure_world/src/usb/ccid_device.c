#include "ccid_device.h"
#include "ccid_protocol.h"
#include "device/usbd_pvt.h"
#include "pico/stdlib.h"
#include "secure_gateway.h"
#include "tusb.h"
#include <stdio.h>

/**
 * @file ccid_device.c
 * @brief USB CCID (Smart Card Reader) class driver implementation.
 *
 * This module emulates a CCID reader and handles communication between the
 * host and the secure world OATH application.
 */

#define CCID_INTERFACE_CLASS 0x0B
#define CCID_DESCRIPTOR_LEN 54
#define YUBIKEY_ATR_LEN 18

// Global state
typedef struct {
  bool is_card_present;
  uint8_t atr[YUBIKEY_ATR_LEN];
  uint8_t ep_out;
  uint8_t ep_in;
  uint8_t rx_buffer[CFG_TUD_CCID_RX_BUFSIZE];
} ccid_context_t;

static ccid_context_t ccid_ctx = {.is_card_present = true,
                                  // ATR: YubiKey 5 Series compatible
                                  .atr = {0x3B, 0xF8, 0x13, 0x00, 0x00, 0x81,
                                          0x31, 0xFE, 0x15, 0x59, 0x75, 0x62,
                                          0x69, 0x4B, 0x65, 0x79, 0x34, 0xD4},
                                  .ep_out = 0,
                                  .ep_in = 0};

// Forward declarations
static void ccid_handle_request(uint8_t const *msg, uint32_t len);

void ccid_init(void) {
  printf("[CCID] Initializing USB Smart Card Interface...\n");
}

//--------------------------------------------------------------------+
// TinyUSB Class Driver Callbacks
//--------------------------------------------------------------------+

static void ccid_user_init(void) { ccid_init(); }

static void ccid_user_reset(uint8_t rhport) {
  (void)rhport;
  ccid_ctx.is_card_present = true;
}

static uint16_t ccid_user_open(uint8_t rhport,
                               tusb_desc_interface_t const *desc_intf,
                               uint16_t max_len) {
  if (desc_intf->bInterfaceClass != CCID_INTERFACE_CLASS)
    return 0;

  uint8_t const *p_desc = (uint8_t const *)desc_intf;
  uint16_t total_len = desc_intf->bLength;
  p_desc += desc_intf->bLength;

  if (total_len + CCID_DESCRIPTOR_LEN > max_len)
    return 0;

  // Skip CCID functional descriptor
  total_len += p_desc[0];
  p_desc += p_desc[0];

  // Open Bulk endpoints
  for (int i = 0; i < 2; i++) {
    tusb_desc_endpoint_t const *desc_ep = (tusb_desc_endpoint_t const *)p_desc;
    if (!usbd_edpt_open(rhport, desc_ep))
      return 0;

    if (desc_ep->bEndpointAddress & 0x80) {
      ccid_ctx.ep_in = desc_ep->bEndpointAddress;
    } else {
      ccid_ctx.ep_out = desc_ep->bEndpointAddress;
    }

    total_len += desc_ep->bLength;
    p_desc += desc_ep->bLength;
  }

  // Start listening
  usbd_edpt_xfer(rhport, ccid_ctx.ep_out, ccid_ctx.rx_buffer,
                 sizeof(ccid_ctx.rx_buffer), false);

  return total_len;
}

static bool ccid_user_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                      tusb_control_request_t const *request) {
  (void)rhport;
  (void)stage;
  return (request->bmRequestType_bit.type == TUSB_REQ_TYPE_CLASS);
}

static bool ccid_user_xfer_cb(uint8_t rhport, uint8_t ep_addr,
                              xfer_result_t result, uint32_t xferred_bytes) {
  if (result != XFER_RESULT_SUCCESS)
    return false;

  if (ep_addr == ccid_ctx.ep_out) {
    ccid_handle_request(ccid_ctx.rx_buffer, xferred_bytes);
    usbd_edpt_xfer(rhport, ccid_ctx.ep_out, ccid_ctx.rx_buffer,
                   sizeof(ccid_ctx.rx_buffer), false);
  }
  return true;
}

const usbd_class_driver_t ccid_driver = {.name = "CCID",
                                         .init = ccid_user_init,
                                         .reset = ccid_user_reset,
                                         .open = ccid_user_open,
                                         .control_xfer_cb =
                                             ccid_user_control_xfer_cb,
                                         .xfer_cb = ccid_user_xfer_cb,
                                         .sof = NULL};

//--------------------------------------------------------------------+
// CCID Protocol Logic
//--------------------------------------------------------------------+

static void ccid_handle_request(uint8_t const *msg, uint32_t len) {
  if (len < CCID_HEADER_SIZE)
    return;

  ccid_msg_header_t const *header = (ccid_msg_header_t const *)msg;
  uint8_t rhport = 0;

  switch (header->bMessageType) {
  case PC_TO_RDR_ICCPOWERON: {
    rdr_to_pc_datablock_t resp = {.bMessageType = RDR_TO_PC_DATABLOCK,
                                  .dwLength = sizeof(ccid_ctx.atr),
                                  .bSlot = header->bSlot,
                                  .bSeq = header->bSeq,
                                  .bStatus = SLOT_STATUS_ICC_PRESENT,
                                  .bError = 0,
                                  .bChainParameter = 0};
    memcpy(resp.abData, ccid_ctx.atr, sizeof(ccid_ctx.atr));
    usbd_edpt_xfer(rhport, ccid_ctx.ep_in, (uint8_t *)&resp,
                   CCID_HEADER_SIZE + sizeof(ccid_ctx.atr), false);
    break;
  }

  case PC_TO_RDR_XFRBLOCK: {
    uint16_t apdu_in_len = (uint16_t)header->dwLength;
    uint8_t apdu_out[MAX_APDU_SIZE + 2];
    uint16_t apdu_out_len = 0;

    // Secure Gateway Call
    secure_gateway_oath_handle_apdu((uint8_t *)msg + CCID_HEADER_SIZE,
                                    apdu_in_len, apdu_out, &apdu_out_len);

    rdr_to_pc_datablock_t resp = {.bMessageType = RDR_TO_PC_DATABLOCK,
                                  .dwLength = apdu_out_len,
                                  .bSlot = header->bSlot,
                                  .bSeq = header->bSeq,
                                  .bStatus = SLOT_STATUS_ICC_PRESENT,
                                  .bError = 0,
                                  .bChainParameter = 0};
    memcpy(resp.abData, apdu_out, apdu_out_len);
    usbd_edpt_xfer(rhport, ccid_ctx.ep_in, (uint8_t *)&resp,
                   CCID_HEADER_SIZE + apdu_out_len, false);
    break;
  }

  case PC_TO_RDR_GETSLOTSTATUS: {
    rdr_to_pc_slotstatus_t resp = {.bMessageType = RDR_TO_PC_SLOTSTATUS,
                                   .dwLength = 0,
                                   .bSlot = header->bSlot,
                                   .bSeq = header->bSeq,
                                   .bStatus = SLOT_STATUS_ICC_PRESENT,
                                   .bError = 0,
                                   .bSpecific = 0};
    usbd_edpt_xfer(rhport, ccid_ctx.ep_in, (uint8_t *)&resp, sizeof(resp),
                   false);
    break;
  }

  case PC_TO_RDR_GETPARAMETERS: {
    uint8_t params[] = {0x01, 0x11, 0x10, 0x10,
                        0x4D, 0x20, 0xFE}; // T=1 Protocol Defaults
    rdr_to_pc_datablock_t resp = {.bMessageType = RDR_TO_PC_PARAMETERS,
                                  .dwLength = sizeof(params),
                                  .bSlot = header->bSlot,
                                  .bSeq = header->bSeq,
                                  .bStatus = SLOT_STATUS_ICC_PRESENT,
                                  .bError = 0,
                                  .bChainParameter = 0};
    memcpy(resp.abData, params, sizeof(params));
    usbd_edpt_xfer(rhport, ccid_ctx.ep_in, (uint8_t *)&resp,
                   CCID_HEADER_SIZE + sizeof(params), false);
    break;
  }

  default:
    printf("[CCID] Unsupported message type: 0x%02X\n", header->bMessageType);
    break;
  }
}

bool tud_ccid_get_atr_cb(uint8_t rhport, uint8_t *buffer, uint16_t *len) {
  (void)rhport;
  *len = sizeof(ccid_ctx.atr);
  memcpy(buffer, ccid_ctx.atr, *len);
  return true;
}

void ccid_task(void) {
  // Background tasks if needed
}
