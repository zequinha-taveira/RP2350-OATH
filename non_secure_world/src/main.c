#include <stdio.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "tusb.h"
#include "usb/ccid_device.h" // Non-Secure USB driver
#include "secure_gateway.h" // Secure Gateway interface

// Main application entry point (Non-Secure World)
int main() {
    // Initialize standard I/O (USB CDC)
    stdio_init_all();

    // 1. Initialize Secure World (via Secure Gateway)
    // This call will jump to the Secure World entry point (main_s)
    secure_gateway_init();

    // 2. Initialize Non-Secure USB (CCID)
    tusb_init();
    ccid_init();

    printf("Non-Secure World: USB/CCID initialized. Waiting for Secure World to boot...\n");

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

// Placeholder for CCID initialization (called from main)
void ccid_init(void) {
    // TinyUSB is initialized in main, this is for CCID-specific setup
    printf("USB: Initializing CCID device...\n");
}

// Placeholder for CCID task (called from main loop)
void ccid_task(void) {
    // TODO: Implement main CCID state machine and APDU processing
}

// Placeholder for TinyUSB callbacks (defined in ccid_device.c)
// void tud_mount_cb(void) {}
// void tud_umount_cb(void) {}
// void tud_suspend_cb(bool remote_wakeup_en) {}
// void tud_resume_cb(void) {}
