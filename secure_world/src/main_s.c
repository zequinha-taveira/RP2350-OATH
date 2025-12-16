#include <stdio.h>
#include "pico/stdlib.h"
#include "oath_protocol.h"
#include "security.h"

// Secure World main entry point
int main_s(void) {
    // This function is the entry point for the Secure World.
    // In a TF-M environment, this would be handled by the TF-M core.
    
    // 1. Initialize secure peripherals (e.g., memory protection)
    // 2. Initialize secure services (OATH, Crypto, Storage)
    security_init();
    oath_init();
    
    printf("Secure World: OATH and Crypto services initialized.\n");
    
    // The Secure World will primarily wait for calls from the Non-Secure World
    // via the Secure Gateway (PPC/SAU configuration).
    
    // TODO: Implement the Secure Gateway interface here.
    
    while (1) {
        // Secure World main loop (mostly idle, waiting for IPC/Secure Gateway calls)
        __wfi(); // Wait for interrupt
    }
    
    return 0;
}
