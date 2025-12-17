#ifndef _USB_COMPOSITE_H_
#define _USB_COMPOSITE_H_

#include "device/usbd_pvt.h"

// Composite USB driver initialization
void usb_composite_init(void);

// Driver instances
extern const usbd_class_driver_t ccid_driver;
extern const usbd_class_driver_t webusb_driver;
extern const usbd_class_driver_t fido2_driver;

#endif // _USB_COMPOSITE_H_