#include "device/usbd_pvt.h"
#include "tusb.h"
#include "usb/ccid_device.h"
#include "usb/fido2_device.h"
#include "usb/webusb_device.h"
#include <stdint.h>
#include <stdio.h>

// Composite USB Device Driver
// Handles multiple interfaces: CCID, WebUSB, and FIDO2

// External driver definitions
extern usbd_class_driver_t const ccid_driver;
extern usbd_class_driver_t const webusb_driver;
extern usbd_class_driver_t const fido2_driver;

static usbd_class_driver_t const *drivers[3];
static uint8_t registered_driver_count = 0;

void usb_composite_init(void) {
  // Registered drivers
  drivers[0] = &ccid_driver;
  drivers[1] = &webusb_driver;
  drivers[2] = &fido2_driver;
  registered_driver_count = 3;

  printf("USB Composite: %d drivers registered\n", registered_driver_count);
}

// This callback is called by TinyUSB stack to get application-defined class
// drivers
usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count) {
  *driver_count = registered_driver_count;
  return drivers[0]; // Return the start of the array
}
