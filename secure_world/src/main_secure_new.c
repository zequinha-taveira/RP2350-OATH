#include <stdio.h>
#include "pico/stdlib.h"
#include "oath_protocol.h"
#include "security.h"
#include "time_sync.h"
#include "hid_keyboard.h"
#include "drivers/led_driver.h"

// Button pin configuration
#define BUTTON_PIN 21
#define LONG_PRESS_MS 3000
#define DOUBLE_PRESS_MS 500

// State machine for modes
typedef enum {
    MODE_CCID,      // CCID mode for Yubico Authenticator
    MODE_HID,       // HID keyboard mode
    MODE_CONFIG     // Configuration mode
} device_mode_t;

static device_mode_t current_mode = MODE_CCID;
static uint64_t last_button_press = 0;
static uint64_t button_press_time = 0;
static bool button_pressed = false;
static bool long_press_detected = false;

// Forward declarations
void handle_button_press(void);
void handle_button_release(void);
void update_led_status(void);

/**
 * @brief Initialize all secure world components
 */
void secure_world_init(void) {
    printf("Secure World: Initializing...\n");
    
    // Initialize hardware security
    security_init();
    
    // Initialize OATH protocol
    oath_init();
    
    // Initialize time sync
    time_sync_init();
    
    // Initialize HID keyboard
    hid_keyboard_init();
    
    // Initialize button
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN); // Active low button
    
    printf("Secure World: All systems initialized\n");
}

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
void handle_button_press(void) {
    uint64_t now = time_us_64();
    uint64_t press_duration = now - button_press_time;
    
    // Check for long press
    if (press_duration > (LONG_PRESS_MS * 1000)) {
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
void handle_button_release(void) {
    uint64_t now = time_us_64();
    uint64_t press_duration = now - button_press_time;
    
    if (!long_press_detected && press_duration > 50000) { // > 50ms
        // Short press
        if (current_mode == MODE_HID) {
            // In HID mode, generate code or cycle credential
            if (button_press_time - last_button_press < (DOUBLE_PRESS_MS * 1000)) {
                // Double press: Cycle credential
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
    
    last_button_press = button_press_time;
    button_pressed = false;
    long_press_detected = false;
}

/**
 * @brief Main loop for secure world
 */
void secure_world_main_loop(void) {
    // Check button state
    bool button_state = !gpio_get(BUTTON_PIN); // Active low
    
    if (button_state && !button_pressed) {
        // Button just pressed
        button_pressed = true;
        button_press_time = time_us_64();
    } else if (!button_state && button_pressed) {
        // Button just released
        handle_button_release();
    } else if (button_state && button_pressed) {
        // Button still pressed
        handle_button_press();
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

/**
 * @brief Secure World main entry point
 */
int main_s(void) {
    // Initialize all systems
    secure_world_init();
    
    printf("Secure World: Ready\n");
    
    // Main loop
    while (1) {
        secure_world_main_loop();
        __wfi(); // Wait for interrupt to save power
    }
    
    return 0;
}