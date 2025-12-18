#include "device/usbd_pvt.h"
#include "tusb.h"
#include "usb/ccid_device.h"
#include "usb/fido2_device.h"
#include "usb/webusb_device.h"
#include <stdint.h>
#include <stdio.h>

/**
 * @file usb_composite.c
 * @brief Management of multiple USB class drivers (CCID, WebUSB, FIDO2).
 */

extern usbd_class_driver_t const ccid_driver;
extern usbd_class_driver_t const webusb_driver;
extern usbd_class_driver_t const fido2_driver;

static usbd_class_driver_t const *drivers[] = {&ccid_driver, &webusb_driver,
                                               &fido2_driver};

#define DRIVER_COUNT (sizeof(drivers) / sizeof(drivers[0]))

void usb_composite_init(void) {
  printf("[USB] Registering %d class drivers (CCID, WebUSB, FIDO2)...\n",
         (int)DRIVER_COUNT);
}

/**
 * TinyUSB callback to get application-defined class drivers.
 * @param driver_count Output for the number of drivers.
 * @return Pointer to the driver array.
 */
usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count) {
  *driver_count = (uint8_t)DRIVER_COUNT;
  return drivers[0];
}
