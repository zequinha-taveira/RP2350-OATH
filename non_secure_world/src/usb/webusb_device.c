#include "webusb_device.h"
#include "device/usbd_pvt.h"
#include "tusb.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

//--------------------------------------------------------------------+
// WebUSB Device State
//--------------------------------------------------------------------+

typedef struct {
    bool is_connected;
    bool is_configured;
    uint8_t ep_out;
    uint8_t ep_in;
    uint8_t rx_buffer[WEBUSB_EP_SIZE];
    uint8_t tx_buffer[WEBUSB_EP_SIZE];
} webusb_state_t;

static webusb_state_t webusb_state = {
    .is_connected = false,
    .is_configured = false,
    .ep_out = 0,
    .ep_in = 0
};

// Forward declarations
void webusb_rx_cb(uint8_t rhport, uint8_t *buffer, uint32_t len);
void webusb_handle_command(uint8_t const *msg, uint32_t len);

//--------------------------------------------------------------------+
// WebUSB Descriptors
//--------------------------------------------------------------------+

// WebUSB Platform Descriptor (Microsoft OS 2.0 descriptor compatible)
#define WEBUSB_PLATFORM_DESCRIPTOR_LENGTH 28

static const uint8_t webusb_platform_descriptor[] = {
    // Length (2 bytes)
    WEBUSB_PLATFORM_DESCRIPTOR_LENGTH & 0xFF, (WEBUSB_PLATFORM_DESCRIPTOR_LENGTH >> 8) & 0xFF,
    // Descriptor Type (0x29 = MS OS 2.0 Platform Capability)
    0x29,
    // Capability Type (0x05 = USB 2.0 Extension)
    0x05,
    // bDevCapabilityData
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};

// WebUSB URL Descriptor
static const uint8_t webusb_url_descriptor[] = {
    // Length + Descriptor Type
    3 + 23, 0x03,  // Length: 26, Type: 0x03 (String)
    // URL: https://localhost:3000
    'h', 't', 't', 'p', 's', ':', '/', '/', 'l', 'o', 'c', 'a', 'l', 'h', 'o', 's', 't', ':', '3', '0', '0', '0'
};

void webusb_init(void) {
    printf("WebUSB: Initialized\n");
    webusb_state.is_connected = false;
    webusb_state.is_configured = false;
}

uint16_t webusb_get_descriptor(uint8_t *buffer, uint16_t bufsize) {
    if (bufsize < sizeof(webusb_platform_descriptor)) {
        return 0;
    }
    memcpy(buffer, webusb_platform_descriptor, sizeof(webusb_platform_descriptor));
    return sizeof(webusb_platform_descriptor);
}

//--------------------------------------------------------------------+
// Custom WebUSB Class Driver
//--------------------------------------------------------------------+

static void webusb_user_init(void) { webusb_init(); }

static void webusb_user_reset(uint8_t rhport) {
    (void)rhport;
    webusb_state.is_connected = false;
    webusb_state.is_configured = false;
}

static uint16_t webusb_user_open(uint8_t rhport,
                                 tusb_desc_interface_t const *desc_intf,
                                 uint16_t max_len) {
    // Verify that the interface is Vendor Specific (0xFF)
    if (desc_intf->bInterfaceClass != 0xFF)
        return 0;

    // Parse endpoints
    uint8_t const *p_desc = (uint8_t const *)desc_intf;
    uint16_t total_len = 0;

    // Skip Interface Descriptor
    p_desc += desc_intf->bLength;
    total_len += desc_intf->bLength;

    // Open Endpoints (Bulk Out and Bulk In)
    for (int i = 0; i < 2; i++) {
        tusb_desc_endpoint_t const *desc_ep = (tusb_desc_endpoint_t const *)p_desc;

        if (desc_ep->bDescriptorType != TUSB_DESC_ENDPOINT)
            return 0;

        if (!usbd_edpt_open(rhport, desc_ep))
            return 0;

        if (desc_ep->bEndpointAddress & 0x80) {
            webusb_state.ep_in = desc_ep->bEndpointAddress;
        } else {
            webusb_state.ep_out = desc_ep->bEndpointAddress;
        }

        p_desc += desc_ep->bLength;
        total_len += desc_ep->bLength;
    }

    // Prepare for RX
    if (!usbd_edpt_xfer(rhport, webusb_state.ep_out, webusb_state.rx_buffer,
                        sizeof(webusb_state.rx_buffer), false)) {
        return 0;
    }

    webusb_state.is_configured = true;
    return total_len;
}

static bool webusb_user_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                        tusb_control_request_t const *request) {
    if (request->bmRequestType_bit.type != TUSB_REQ_TYPE_VENDOR)
        return false;

    // Handle WebUSB specific requests
    switch (request->bRequest) {
        case WEBUSB_REQ_GET_URL:
            if (stage == CONTROL_STAGE_SETUP) {
                // Send URL descriptor
                uint16_t len = sizeof(webusb_url_descriptor);
                if (len > request->wLength) {
                    len = request->wLength;
                }
                return tud_control_xfer(rhport, request, (void*)webusb_url_descriptor, len);
            }
            break;
            
        case WEBUSB_REQ_GET_STATUS:
            if (stage == CONTROL_STAGE_SETUP) {
                uint8_t status = webusb_state.is_connected ? 0x01 : 0x00;
                return tud_control_xfer(rhport, request, &status, 1);
            }
            break;
            
        case WEBUSB_REQ_GET_CONFIG:
            if (stage == CONTROL_STAGE_SETUP) {
                uint8_t config = webusb_state.is_configured ? 0x01 : 0x00;
                return tud_control_xfer(rhport, request, &config, 1);
            }
            break;
            
        case WEBUSB_REQ_SET_CONFIG:
            if (stage == CONTROL_STAGE_SETUP) {
                webusb_state.is_connected = true;
                printf("WebUSB: Connected and configured\n");
                uint8_t ack = 0x00;
                return tud_control_xfer(rhport, request, &ack, 1);
            }
            break;
    }

    return false;
}

static bool webusb_user_xfer_cb(uint8_t rhport, uint8_t ep_addr,
                                xfer_result_t result, uint32_t xferred_bytes) {
    (void)rhport;
    
    if (result != XFER_RESULT_SUCCESS)
        return false;

    if (ep_addr == webusb_state.ep_out) {
        // Handle received data
        webusb_rx_cb(rhport, webusb_state.rx_buffer, xferred_bytes);
        
        // Prepare for next RX
        usbd_edpt_xfer(rhport, webusb_state.ep_out, webusb_state.rx_buffer,
                       sizeof(webusb_state.rx_buffer), false);
    }
    
    return true;
}

static const usbd_class_driver_t webusb_driver = {
    .name = "WebUSB",
    .init = webusb_user_init,
    .reset = webusb_user_reset,
    .open = webusb_user_open,
    .control_xfer_cb = webusb_user_control_xfer_cb,
    .xfer_cb = webusb_user_xfer_cb,
    .sof = NULL
};

usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count) {
    // This will be called by TinyUSB to get the driver
    // We need to return both CCID and WebUSB drivers
    // This function will be modified in the main app to handle multiple drivers
    return &webusb_driver;
}

bool webusb_send_response(uint8_t const *buffer, uint16_t length) {
    if (!webusb_state.is_configured || webusb_state.ep_in == 0)
        return false;

    if (length > sizeof(webusb_state.tx_buffer))
        length = sizeof(webusb_state.tx_buffer);

    memcpy(webusb_state.tx_buffer, buffer, length);
    
    // Use TinyUSB to send data
    return tud_vendor_write(webusb_state.tx_buffer, length);
}

void webusb_rx_cb(uint8_t rhport, uint8_t *buffer, uint32_t len) {
    printf("WebUSB: Received %lu bytes\n", len);
    webusb_handle_command(buffer, len);
}

void webusb_handle_command(uint8_t const *msg, uint32_t len) {
    if (len < 1) return;

    uint8_t command = msg[0];
    uint8_t response[64];
    uint16_t response_len = 0;

    printf("WebUSB: Command 0x%02X received\n", command);

    switch (command) {
        case WEBUSB_CMD_PING:
            response[0] = WEBUSB_CMD_PING;
            response[1] = WEBUSB_STATUS_OK;
            response[2] = 0x00; // Reserved
            response_len = 3;
            break;

        case WEBUSB_CMD_GET_INFO:
            response[0] = WEBUSB_CMD_GET_INFO;
            response[1] = WEBUSB_STATUS_OK;
            // Device info
            response[2] = 0x01; // Version major
            response[3] = 0x00; // Version minor
            response[4] = 0x00; // Version patch
            response[5] = 0x01; // Capabilities: WebUSB
            response[6] = 0x02; // Capabilities: FIDO2
            response_len = 7;
            break;

        case WEBUSB_CMD_GET_CONFIG:
            response[0] = WEBUSB_CMD_GET_CONFIG;
            response[1] = WEBUSB_STATUS_OK;
            // Current configuration
            response[2] = webusb_state.is_connected ? 0x01 : 0x00;
            response[3] = webusb_state.is_configured ? 0x01 : 0x00;
            response_len = 4;
            break;

        case WEBUSB_CMD_SET_CONFIG:
            if (len >= 2) {
                uint8_t config = msg[1];
                webusb_state.is_connected = (config & 0x01) != 0;
                response[0] = WEBUSB_CMD_SET_CONFIG;
                response[1] = WEBUSB_STATUS_OK;
                response_len = 2;
            } else {
                response[0] = WEBUSB_CMD_SET_CONFIG;
                response[1] = WEBUSB_STATUS_INVALID;
                response_len = 2;
            }
            break;

        case WEBUSB_CMD_RESET:
            response[0] = WEBUSB_CMD_RESET;
            response[1] = WEBUSB_STATUS_OK;
            response_len = 2;
            // Reset state
            webusb_state.is_connected = false;
            break;

        default:
            response[0] = command;
            response[1] = WEBUSB_STATUS_INVALID;
            response_len = 2;
            break;
    }

    if (response_len > 0) {
        webusb_send_response(response, response_len);
    }
}

void webusb_task(void) {
    // WebUSB task handler (if needed)
    // Currently handled via callbacks
}