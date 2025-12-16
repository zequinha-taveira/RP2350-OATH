#include "ccid_device.h"     // Custom header for CCID definitions
#include "ccid_protocol.h"   // CCID message structures and constants
#include "device/usbd_pvt.h" // For usbd_class_driver_t
#include "pico/stdlib.h"
#include "secure_gateway.h" // For Secure Gateway
#include "tusb.h"
#include <stdio.h>

//--------------------------------------------------------------------+
// CCID Device Implementation
//--------------------------------------------------------------------+

// Global buffer for incoming CCID messages
uint8_t ccid_rx_buffer[CFG_TUD_CCID_RX_BUFSIZE];

// CCID State
typedef struct {
  bool is_card_present;
  uint8_t atr[32]; // Answer To Reset
  uint8_t ep_out;
  uint8_t ep_in;
} ccid_state_t;

static ccid_state_t ccid_state = {
    .is_card_present = true, // Always present for emulated token
    .atr = {0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00},
    .ep_out = 0,
    .ep_in = 0};

// Forward declaration for CCID message handler
void ccid_message_handler(uint8_t const *msg, uint32_t len);

// Forward declaration for RX callback (internal usage)
void tud_ccid_rx_cb(uint8_t rhport, uint8_t *buffer, uint32_t len);

void ccid_init(void) { printf("CCID: Initialized\n"); }

//--------------------------------------------------------------------+
// Custom CCID Class Driver
//--------------------------------------------------------------------+

static void ccid_user_init(void) { ccid_init(); }

static void ccid_user_reset(uint8_t rhport) {
  (void)rhport;
  ccid_state.is_card_present = true;
  // Endpoints are closed by USBD core
}

static uint16_t ccid_user_open(uint8_t rhport,
                               tusb_desc_interface_t const *desc_intf,
                               uint16_t max_len) {
  // 1. Verify that the interface is CCID (Smart Card)
  if (desc_intf->bInterfaceClass != 0x0B)
    return 0;

  // 2. Parse the Functional Descriptor (CCID Descriptor)
  uint8_t const *p_desc = (uint8_t const *)desc_intf;
  uint16_t total_len = 0;

  // Skip Interface Descriptor
  p_desc += desc_intf->bLength;
  total_len += desc_intf->bLength;

  if (total_len + 54 > max_len)
    return 0;

  uint8_t const *ccid_desc = p_desc;
  // Advance by descriptor length (likely 54)
  p_desc += ccid_desc[0];
  total_len += ccid_desc[0];

  // 3. Open Endpoints (Bulk Out and Bulk In)
  for (int i = 0; i < 2; i++) {
    tusb_desc_endpoint_t const *desc_ep = (tusb_desc_endpoint_t const *)p_desc;

    if (desc_ep->bDescriptorType != TUSB_DESC_ENDPOINT)
      return 0;

    if (!usbd_edpt_open(rhport, desc_ep))
      return 0;

    if (desc_ep->bEndpointAddress & 0x80) {
      ccid_state.ep_in = desc_ep->bEndpointAddress;
    } else {
      ccid_state.ep_out = desc_ep->bEndpointAddress;
    }

    p_desc += desc_ep->bLength;
    total_len += desc_ep->bLength;
  }

  // 4. Prepare for RX
  if (!usbd_edpt_xfer(rhport, ccid_state.ep_out, ccid_rx_buffer,
                      sizeof(ccid_rx_buffer), false)) {
    return 0;
  }

  return total_len;
}

static bool ccid_user_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                      tusb_control_request_t const *request) {
  if (request->bmRequestType_bit.type != TUSB_REQ_TYPE_CLASS)
    return false;
  return true;
}

static bool ccid_user_xfer_cb(uint8_t rhport, uint8_t ep_addr,
                              xfer_result_t result, uint32_t xferred_bytes) {
  (void)rhport;
  if (result != XFER_RESULT_SUCCESS)
    return false;

  if (ep_addr == ccid_state.ep_out) {
    tud_ccid_rx_cb(rhport, ccid_rx_buffer, xferred_bytes);
    usbd_edpt_xfer(rhport, ccid_state.ep_out, ccid_rx_buffer,
                   sizeof(ccid_rx_buffer), false);
  }
  return true;
}

static const usbd_class_driver_t ccid_driver = {.name = "CCID",
                                                .init = ccid_user_init,
                                                .reset = ccid_user_reset,
                                                .open = ccid_user_open,
                                                .control_xfer_cb =
                                                    ccid_user_control_xfer_cb,
                                                .xfer_cb = ccid_user_xfer_cb,
                                                .sof = NULL};

usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count) {
  *driver_count = 1;
  return &ccid_driver;
}

bool tud_ccid_tx(uint8_t rhport, uint8_t const *buffer, uint32_t bufsize) {
  if (ccid_state.ep_in == 0)
    return false;
  return usbd_edpt_xfer(rhport, ccid_state.ep_in, (uint8_t *)buffer,
                        (uint16_t)bufsize, false);
}

//--------------------------------------------------------------------+
// RX Callback
//--------------------------------------------------------------------+

void tud_ccid_rx_cb(uint8_t rhport, uint8_t *buffer, uint32_t len) {
  ccid_message_handler(buffer, len);
}

//--------------------------------------------------------------------+
// CCID Message Handler
//--------------------------------------------------------------------+

void ccid_message_handler(uint8_t const *msg, uint32_t len) {
  ccid_msg_header_t const *header = (ccid_msg_header_t const *)msg;
  uint8_t rhport = 0;

  if (len < CCID_HEADER_SIZE) {
    printf("CCID: ERROR - Message too short (%lu bytes)\n", len);
    return;
  }

  printf("CCID: RX Type=0x%02X, Len=%lu, Slot=%u, Seq=%u\n",
         header->bMessageType, header->dwLength, header->bSlot, header->bSeq);

  switch (header->bMessageType) {
  case PC_TO_RDR_ICCPOWERON: {
    rdr_to_pc_slotstatus_t poweron_resp = {.bMessageType = RDR_TO_PC_SLOTSTATUS,
                                           .dwLength = 0,
                                           .bSlot = header->bSlot,
                                           .bSeq = header->bSeq,
                                           .bStatus = SLOT_STATUS_ICC_PRESENT,
                                           .bError = 0,
                                           .bSpecific = 0};
    tud_ccid_tx(rhport, (uint8_t const *)&poweron_resp, sizeof(poweron_resp));
    break;
  }

  case PC_TO_RDR_XFRBLOCK: {
    uint8_t const *apdu_command = msg + sizeof(ccid_msg_header_t);
    uint16_t apdu_len = header->dwLength;

    uint8_t apdu_response[MAX_APDU_SIZE + 2];
    uint16_t response_len = 0;

    // Call OATH handler via Secure Gateway
    secure_gateway_oath_handle_apdu((uint8_t *)apdu_command, apdu_len,
                                    apdu_response, &response_len);

    rdr_to_pc_datablock_t xfr_resp = {.bMessageType = RDR_TO_PC_DATABLOCK,
                                      .dwLength = response_len,
                                      .bSlot = header->bSlot,
                                      .bSeq = header->bSeq,
                                      .bStatus = SLOT_STATUS_ICC_PRESENT,
                                      .bError = 0,
                                      .bChainParameter = 0};

    memcpy(xfr_resp.abData, apdu_response, response_len);

    size_t total_len = sizeof(ccid_msg_header_t) + 3 + response_len;
    tud_ccid_tx(rhport, (uint8_t const *)&xfr_resp, total_len);
    break;
  }

  case PC_TO_RDR_ICCPOWEROFF: {
    rdr_to_pc_slotstatus_t poweroff_resp = {.bMessageType =
                                                RDR_TO_PC_SLOTSTATUS,
                                            .dwLength = 0,
                                            .bSlot = header->bSlot,
                                            .bSeq = header->bSeq,
                                            .bStatus = SLOT_STATUS_ICC_ABSENT,
                                            .bError = 0,
                                            .bSpecific = 0};
    tud_ccid_tx(rhport, (uint8_t const *)&poweroff_resp, sizeof(poweroff_resp));
    break;
  }

  case PC_TO_RDR_GETSLOTSTATUS: {
    rdr_to_pc_slotstatus_t status_resp = {.bMessageType = RDR_TO_PC_SLOTSTATUS,
                                          .dwLength = 0,
                                          .bSlot = header->bSlot,
                                          .bSeq = header->bSeq,
                                          .bStatus = SLOT_STATUS_ICC_PRESENT,
                                          .bError = 0,
                                          .bSpecific = 0};
    tud_ccid_tx(rhport, (uint8_t const *)&status_resp, sizeof(status_resp));
    break;
  }

  default:
    printf("CCID: Unknown message type 0x%02X\n", header->bMessageType);
    break;
  }
}

uint16_t tud_ccid_get_descriptor_cb(uint8_t *buffer, uint16_t bufsize) {
  return 0;
}

bool tud_ccid_get_atr_cb(uint8_t rhport, uint8_t *buffer, uint16_t *len) {
  *len = sizeof(ccid_state.atr);
  memcpy(buffer, ccid_state.atr, *len);
  return true;
}

bool tud_ccid_get_slot_status_cb(uint8_t rhport, uint8_t *buffer,
                                 uint16_t *len) {
  return true;
}

void ccid_task(void) {}
