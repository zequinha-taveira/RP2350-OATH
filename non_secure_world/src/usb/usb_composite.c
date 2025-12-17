#include "tusb.h"
#include "device/usbd_pvt.h"
#include "usb/ccid_device.h"
#include "usb/webusb_device.h"
#include "usb/fido2_device.h"
#include <stdio.h>

// Composite USB Device Driver
// Handles multiple interfaces: CCID, WebUSB, and FIDO2

static uint8_t driver_count = 0;
static usbd_class_driver_t const *drivers[3];

void usb_composite_init(void) {
    // Initialize all drivers
    ccid_init();
    webusb_init();
    fido2_init();
    
    // Register drivers
    drivers[0] = &ccid_driver;
    drivers[1] = &webusb_driver;
    drivers[2] = &fido2_driver;
    driver_count = 3;
    
    printf("USB Composite: %d drivers registered\n", driver_count);
}

usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count) {
    *driver_count = driver_count;
    return drivers;
}

// Global driver instances
// These need to be defined here since they're referenced by the composite driver

static void ccid_user_init(void) { ccid_init(); }
static void ccid_user_reset(uint8_t rhport) {
    (void)rhport;
    // Reset CCID state
}
static uint16_t ccid_user_open(uint8_t rhport, tusb_desc_interface_t const *desc_intf, uint16_t max_len) {
    // CCID open implementation
    return 0;
}
static bool ccid_user_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) {
    return false;
}
static bool ccid_user_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) {
    return false;
}
static void ccid_user_sof(uint8_t rhport) {
    (void)rhport;
}

const usbd_class_driver_t ccid_driver = {
    .name = "CCID",
    .init = ccid_user_init,
    .reset = ccid_user_reset,
    .open = ccid_user_open,
    .control_xfer_cb = ccid_user_control_xfer_cb,
    .xfer_cb = ccid_user_xfer_cb,
    .sof = ccid_user_sof
};

static void webusb_user_init(void) { webusb_init(); }
static void webusb_user_reset(uint8_t rhport) {
    (void)rhport;
}
static uint16_t webusb_user_open(uint8_t rhport, tusb_desc_interface_t const *desc_intf, uint16_t max_len) {
    return 0;
}
static bool webusb_user_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) {
    return false;
}
static bool webusb_user_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) {
    return false;
}
static void webusb_user_sof(uint8_t rhport) {
    (void)rhport;
}

const usbd_class_driver_t webusb_driver = {
    .name = "WebUSB",
    .init = webusb_user_init,
    .reset = webusb_user_reset,
    .open = webusb_user_open,
    .control_xfer_cb = webusb_user_control_xfer_cb,
    .xfer_cb = webusb_user_xfer_cb,
    .sof = webusb_user_sof
};

static void fido2_user_init(void) { fido2_init(); }
static void fido2_user_reset(uint8_t rhport) {
    (void)rhport;
}
static uint16_t fido2_user_open(uint8_t rhport, tusb_desc_interface_t const *desc_intf, uint16_t max_len) {
    return 0;
}
static bool fido2_user_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) {
    return false;
}
static bool fido2_user_xfer_cb(uint8_t rhport, uint8_t ep_addr, xfer_result_t result, uint32_t xferred_bytes) {
    return false;
}
static void fido2_user_sof(uint8_t rhport) {
    (void)rhport;
}

const usbd_class_driver_t fido2_driver = {
    .name = "FIDO2",
    .init = fido2_user_init,
    .reset = fido2_user_reset,
    .open = fido2_user_open,
    .control_xfer_cb = fido2_user_control_xfer_cb,
    .xfer_cb = fido2_user_xfer_cb,
    .sof = fido2_user_sof
};