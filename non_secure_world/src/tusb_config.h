#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
 extern "C" {
#endif

//--------------------------------------------------------------------
// COMMON CONFIGURATION
//--------------------------------------------------------------------

// USB is full speed (12Mhz)
#define CFG_TUSB_RHPORT0_MODE       (OPT_MODE_DEVICE | OPT_MODE_FULL_SPEED)

// Device mode only
#define CFG_TUD_ENABLED             1

//--------------------------------------------------------------------
// DEVICE CONFIGURATION
//--------------------------------------------------------------------

// Number of device configurations
#define CFG_TUD_MAX_SPEED           (TUSB_SPEED_FULL)
#define CFG_TUD_CONFIG_DESC_LEN     (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_MSC_DESC_LEN + TUD_HID_DESC_LEN + TUD_CCID_DESC_LEN)

// Number of Endpoints required
#define CFG_TUD_ENDPOINT_MAX        4

//------------- CCID -------------//
// Custom CCID class (Smart Card Reader) is required for Yubico Authenticator compatibility.
// Class: 0x0B (Smart Card)
#define CFG_TUD_CCID                1
#define CFG_TUD_CCID_RX_BUFSIZE     64  // Required for CCID protocol
#define CFG_TUD_CCID_TX_BUFSIZE     64  // Required for CCID protocol

//------------- CDC (Serial) -------------//
// Used for debug logging
#define CFG_TUD_CDC                 1
#define CFG_TUD_CDC_RX_BUFSIZE      64
#define CFG_TUD_CDC_TX_BUFSIZE      64

//------------- HID (FIDO2) -------------//
// Required for FIDO2/U2F support
#define CFG_TUD_HID                 1
#define CFG_TUD_HID_RX_BUFSIZE      64
#define CFG_TUD_HID_TX_BUFSIZE      64

//------------- MSC (Optional for UF2) -------------//
// Not needed for final firmware, but useful for development
#define CFG_TUD_MSC                 0

//------------- VENDOR (WebUSB) -------------//
// Used for WebUSB interface
#define CFG_TUD_VENDOR              1
#define CFG_TUD_VENDOR_RX_BUFSIZE   64
#define CFG_TUD_VENDOR_TX_BUFSIZE   64

//--------------------------------------------------------------------
// USB DESCRIPTOR CONFIGURATION
//--------------------------------------------------------------------

// Vendor ID and Product ID (Use a custom one for the project)
#define CFG_TUD_VID                 0x1209 // USB Implementers Forum, Inc. (TID)
#define CFG_TUD_PID                 0x4D41 // 'MA' for Manus AI (Placeholder)

// USB Device Class (Composite Device)
#define CFG_TUD_DESC_DEVICE_CLASS   0x00
#define CFG_TUD_DESC_DEVICE_SUBCLASS 0x00
#define CFG_TUD_DESC_DEVICE_PROTOCOL 0x00

#ifdef __cplusplus
 }
#endif

#endif /* _TUSB_CONFIG_H_ */
