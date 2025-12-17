#ifndef _WEBUSB_DEVICE_H_
#define _WEBUSB_DEVICE_H_

#include <stdbool.h>
#include <stdint.h>


// WebUSB Vendor Code definitions
#define WEBUSB_REQ_GET_URL 0x02
#define WEBUSB_REQ_GET_STATUS 0x03
#define WEBUSB_REQ_GET_CONFIG 0x04
#define WEBUSB_REQ_SET_CONFIG 0x05

// WebUSB Landing Page URL
#define WEBUSB_URL_SCHEME_HTTPS 0x02
#define WEBUSB_URL_PREFIX "https://localhost:3000"

// WebUSB Configuration
#define WEBUSB_EP_OUT 0x03
#define WEBUSB_EP_IN 0x83
#define WEBUSB_EP_SIZE 64

// WebUSB Command IDs
#define WEBUSB_CMD_PING 0x01
#define WEBUSB_CMD_GET_INFO 0x02
#define WEBUSB_CMD_GET_CONFIG 0x03
#define WEBUSB_CMD_SET_CONFIG 0x04
#define WEBUSB_CMD_RESET 0x05
#define WEBUSB_CMD_HSM_GEN_KEY 0x10
#define WEBUSB_CMD_HSM_GET_PUBKEY 0x11
#define WEBUSB_CMD_HSM_SIGN 0x12

// WebUSB Response Status
#define WEBUSB_STATUS_OK 0x00
#define WEBUSB_STATUS_ERROR 0x01
#define WEBUSB_STATUS_BUSY 0x02
#define WEBUSB_STATUS_INVALID 0x03

// Function prototypes
void webusb_init(void);
void webusb_task(void);
bool webusb_send_response(uint8_t const *buffer, uint16_t length);
void webusb_message_handler(uint8_t const *msg, uint32_t len);

// WebUSB descriptor helpers
uint16_t webusb_get_descriptor(uint8_t *buffer, uint16_t bufsize);

#endif // _WEBUSB_DEVICE_H_