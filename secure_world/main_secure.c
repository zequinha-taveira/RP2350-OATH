#include "hardware/structs/sio.h"
#include "oath_protocol.h"
#include "pico/stdlib.h"
#include "secure_functions.h"
#include "security.h"
#include "time_sync.h"
#include "hid_keyboard.h"
#include "drivers/led_driver.h"
#include <stdint.h>
#include <stdio.h>

// Define the assumed location of Non-Secure Flash and System Control Block
// This matches memmap_ns.ld: 0x10040000
#define NON_SECURE_FLASH_OFFSET 0x10040000
#define VTOR_OFFSET (0xE000ED08)

// Button pin configuration
#define BUTTON_PIN 21

// State machine for modes
typedef enum {
    MODE_CCID,      // CCID mode for Yubico Authenticator
    MODE_HID,       // HID keyboard mode
    MODE_CONFIG     // Configuration mode
} device_mode_t;

static device_mode_t current_mode = MODE_CCID;
static uint64_t last_button_press = 0;
static bool button_pressed = false;
static bool long_press_detected = false;

// Forward declarations
void handle_button_press(uint64_t press_time);
void handle_button_release(uint64_t release_time);
void update_led_status(void);
void secure_world_main_loop(void);

/**
 * @brief Update LED based on current mode and status
 */
void update_led_status(void) {
    switch (current_mode) {
        case MODE_CCID:
            if (time_sync_is_synced()) {
                led_set_color(0, 255, 0); // Green - Ready
            } else {
                led_set_color(255, 255, 0); // Yellow - No time sync
            }
            break;
            
        case MODE_HID:
            if (time_sync_is_synced()) {
                led_set_color(0, 0, 255); // Blue - HID mode
            } else {
                led_set_color(255, 0, 255); // Magenta - HID no time
            }
            break;
            
        case MODE_CONFIG:
            led_set_color(255, 128, 0); // Orange - Config mode
            break;
    }
}

/**
 * @brief Handle button press event
 */
void handle_button_press(uint64_t press_time) {
    // Check for long press (3 seconds)
    if (press_time > 3000000) { // 3 seconds in microseconds
        if (!long_press_detected) {
            long_press_detected = true;
            
            // Long press: Cycle modes
            current_mode = (current_mode + 1) % 3;
            
            // Visual feedback
            for (int i = 0; i < 3; i++) {
                led_set_color(255, 255, 255);
                sleep_ms(100);
                led_set_color(0, 0, 0);
                sleep_ms(100);
            }
            
            printf("Secure World: Mode changed to %d\n", current_mode);
            update_led_status();
        }
    }
}

/**
 * @brief Handle button release event
 */
void handle_button_release(uint64_t release_time) {
    if (!long_press_detected && release_time > 50000) { // > 50ms
        // Short press
        if (current_mode == MODE_HID) {
            // In HID mode, generate code or cycle credential
            if (last_button_press > 0 &&
                (release_time - last_button_press) < 500000) { // Double press
                hid_keyboard_next_credential();
            } else {
                // Single press: Generate code
                hid_keyboard_generate_code();
            }
        } else if (current_mode == MODE_CCID) {
            // In CCID mode, just blink to acknowledge
            led_set_color(255, 255, 255);
            sleep_ms(100);
            update_led_status();
        }
    }
    
    last_button_press = release_time;
    button_pressed = false;
    long_press_detected = false;
}

/**
 * @brief Main loop for secure world
 */
void secure_world_main_loop(void) {
    // Check button state
    bool button_state = !gpio_get(BUTTON_PIN); // Active low
    
    static uint64_t button_press_time = 0;
    
    if (button_state && !button_pressed) {
        // Button just pressed
        button_pressed = true;
        button_press_time = time_us_64();
    } else if (!button_state && button_pressed) {
        // Button just released
        uint64_t release_time = time_us_64() - button_press_time;
        handle_button_release(release_time);
    } else if (button_state && button_pressed) {
        // Button still pressed
        uint64_t press_duration = time_us_64() - button_press_time;
        handle_button_press(press_duration);
    }
    
    // Update LED status periodically
    static uint32_t last_update = 0;
    uint32_t now = (uint32_t)(time_us_64() / 1000000);
    if (now - last_update > 1) {
        update_led_status();
        last_update = now;
        
        // Debug output
        if (current_mode == MODE_CCID) {
            printf("Secure World: CCID Mode, Time Synced: %s\n",
                   time_sync_is_synced() ? "Yes" : "No");
        } else if (current_mode == MODE_HID) {
            const char* cred = hid_keyboard_get_current_credential();
            printf("Secure World: HID Mode, Credential: %s\n",
                   cred ? cred : "None");
        }
    }
}

/*
 * Secure World Entry Point
 */
int main(void) {
  // 1. Initialize the SDK (clocks, stdio, etc.)
  stdio_init_all();

  printf("Secure World: Initialized.\n");
  printf("Secure World: Configuring TrustZone...\n");

  // 2. Initialize Secure Services
  security_init();
  oath_init();
  time_sync_init();
  hid_keyboard_init();
  
  // Initialize button
  gpio_init(BUTTON_PIN);
  gpio_set_dir(BUTTON_PIN, GPIO_IN);
  gpio_pull_up(BUTTON_PIN); // Active low button
  
  printf("Secure World: All subsystems initialized.\n");

  // 3. TrustZone Configuration (SAU, IDAU) is often handled by bootrom or early
  // startup, but here we ensure specific peripherals/memory regions are
  // configured if needed. For RP2350, much of this is done via the Access
  // Control Lists (ACLs) or SAU. (Simulated for this skeleton)

  printf("Secure World: Preparing to jump to Non-Secure World at 0x%08X...\n",
         NON_SECURE_FLASH_OFFSET);

  // 4. Locate the Non-Secure Vector Table
  // The first word in the vector table is the initial Stack Pointer (SP)
  // The second word is the Reset Handler address
  uint32_t *ns_vector_table = (uint32_t *)NON_SECURE_FLASH_OFFSET;
  uint32_t ns_stack_pointer = ns_vector_table[0];
  uint32_t ns_reset_handler = ns_vector_table[1];

  if (ns_stack_pointer == 0xFFFFFFFF || ns_reset_handler == 0xFFFFFFFF) {
    printf("Secure World: Error! Invalid NS Vector Table at 0x%08X\n",
           NON_SECURE_FLASH_OFFSET);
    printf("Secure World: Halting.\n");
    while (1)
      tight_loop_contents();
  }

  // 5. Set the Non-Secure Vector Table Offset Register (VTOR)
  // On Cortex-M33 (RP2350), the Non-Secure VTOR is aliased at 0xE002ED08
  // when accessed from Secure state.
  // We must point this to the Non-Secure Vector Table in Flash.
  volatile uint32_t *ns_vtor = (volatile uint32_t *)0xE002ED08;
  *ns_vtor = NON_SECURE_FLASH_OFFSET;

  printf("Secure World: Set NS VTOR to 0x%08X\n", NON_SECURE_FLASH_OFFSET);

  // 6. Jump to Non-Secure World
  // We use the CMSE intrinsic to perform the state transition.
  // `cmse_nsfptr_create` creates a function pointer with the LSB unset
  // (indicating NS intent). But typically we just cast the address. The
  // critical part is using `__cmse_nonsecure_call` types if we were CALLING a
  // function and returning. For a one-way jump (reset), we can just branch to
  // the address with LSB unset? Actually, LSB MUST be unset for NS destination
  // in BLXNS.

  printf("Secure World: Jump to NS Reset Handler 0x%08X, SP 0x%08X\n",
         ns_reset_handler, ns_stack_pointer);

  // Flush stdio
  stdio_flush();

  // Inline assembly to perform the jump
  // We load SP_NS and then BLXNS to the reset handler.
  __asm volatile("msr msp_ns, %0\n\t" // Set Non-Secure Main Stack Pointer
                 "bxns %1\n\t"        // Branch and Exchange to Non-Secure state
                 :
                 : "r"(ns_stack_pointer), "r"(ns_reset_handler)
                 : "memory");

  // Should never get here
  while (1) {
    // If we get here, something went wrong
    // Run the main loop instead
    secure_world_main_loop();
  }
  return 0;
}
