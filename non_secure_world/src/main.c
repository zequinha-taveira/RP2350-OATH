#include "pico/stdlib.h"
#include <stdio.h>

#include "secure_functions.h" // NSC functions
#include "secure_gateway.h"   // Secure Gateway interface (SG_INIT)
#include "tusb.h"
#include "usb/ccid_device.h" // Non-Secure USB driver


// Main application entry point (Non-Secure World)
int main(void) {
  // Initialize standard I/O (USB CDC)
  stdio_init_all();

  printf("Non-Secure World: Booting...\n");

  // 1. Initialize Secure World (via Secure Gateway)
  secure_world_handler(SG_INIT, NULL, 0, NULL);
  printf("Non-Secure World: Secure World Initialized.\n");

  // 2. Initialize Non-Secure USB (CCID)
  tusb_init();
  ccid_init();

  printf("Non-Secure World: USB/CCID initialized. Waiting for Secure World to "
         "boot...\n");

  // Main loop
  while (1) {
    // TinyUSB task handler
    tud_task();

    // CCID device task (handles incoming APDU commands)
    ccid_task();

    // Put core to sleep or run low-priority tasks
    tight_loop_contents();
  }

  return 0;
}
