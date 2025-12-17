#include "pico/stdlib.h"
#include <stdio.h>

#include "secure_functions.h" // NSC functions
#include "secure_gateway.h"   // Secure Gateway interface (SG_INIT)
#include "tusb.h"
#include "usb/usb_composite.h" // Composite USB driver
#include "usb/ccid_device.h"   // CCID driver
#include "usb/webusb_device.h" // WebUSB driver
#include "usb/fido2_device.h"  // FIDO2 driver


// Main application entry point (Non-Secure World)
int main(void) {
  // Initialize standard I/O (USB CDC)
  stdio_init_all();

  printf("Non-Secure World: Booting...\n");

  // 1. Initialize Secure World (via Secure Gateway)
  secure_world_handler(SG_INIT, NULL, 0, NULL);
  printf("Non-Secure World: Secure World Initialized.\n");

  // 2. Initialize all USB interfaces
  tusb_init();
  
  // Initialize individual device drivers
  ccid_init();
  webusb_init();
  fido2_init();
  
  // Initialize composite driver
  usb_composite_init();

  printf("Non-Secure World: USB Composite Device initialized:\n");
  printf("  - CCID (Yubico Authenticator)\n");
  printf("  - WebUSB (Advanced Configuration)\n");
  printf("  - FIDO2/U2F (Passwordless Auth)\n");
  printf("Waiting for Secure World to boot...\n");

  // Main loop
  while (1) {
    // TinyUSB task handler
    tud_task();

    // Device tasks
    ccid_task();
    webusb_task();
    fido2_task();

    // Put core to sleep or run low-priority tasks
    tight_loop_contents();
  }

  return 0;
}
