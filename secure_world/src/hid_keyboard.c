#include "hid_keyboard.h"
#include "oath_storage.h"
#include "time_sync.h"
#include "led_driver.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

// HID Keyboard state
static bool hid_mode_enabled = false;
static uint8_t current_credential_index = 0;
static uint64_t last_button_press = 0;

// HID key codes for digits 0-9
static const uint8_t key_codes[10] = {
    0x27, // 0
    0x1E, // 1
    0x1F, // 2
    0x20, // 3
    0x21, // 4
    0x22, // 5
    0x23, // 6
    0x24, // 7
    0x25, // 8
    0x26  // 9
};

// HID key code for Enter
#define KEY_ENTER 0x28

/**
 * @brief Initialize HID Keyboard mode
 */
void hid_keyboard_init(void) {
    hid_mode_enabled = false;
    current_credential_index = 0;
    last_button_press = 0;
    printf("HID Keyboard: Initialized\n");
}

/**
 * @brief Enable/disable HID Keyboard mode
 */
void hid_keyboard_set_mode(bool enabled) {
    hid_mode_enabled = enabled;
    if (enabled) {
        led_set_color(0, 255, 0); // Green for HID mode
        printf("HID Keyboard: Mode enabled\n");
    } else {
        led_set_color(0, 0, 0); // Off
        printf("HID Keyboard: Mode disabled\n");
    }
}

/**
 * @brief Get current HID mode
 */
bool hid_keyboard_get_mode(void) {
    return hid_mode_enabled;
}

/**
 * @brief Cycle to next credential
 */
void hid_keyboard_next_credential(void) {
    const char* name = oath_storage_list(current_credential_index);
    if (name == NULL) {
        current_credential_index = 0;
    } else {
        current_credential_index++;
    }
    
    // Blink LED to indicate change
    led_set_color(255, 255, 0); // Yellow
    sleep_ms(100);
    led_set_color(0, 255, 0); // Back to green
}

/**
 * @brief Generate and type TOTP code for current credential
 * @return true if successful, false otherwise
 */
bool hid_keyboard_generate_code(void) {
    if (!hid_mode_enabled) {
        return false;
    }
    
    // Get current credential name
    const char* name = oath_storage_list(current_credential_index);
    if (name == NULL) {
        // No credentials available
        led_set_color(255, 0, 0); // Red
        sleep_ms(200);
        led_set_color(0, 255, 0); // Green
        return false;
    }
    
    // Get credential details
    oath_credential_t cred;
    if (!oath_storage_get(name, &cred)) {
        return false;
    }
    
    // Check if time is synced (required for TOTP)
    if (!time_sync_is_synced()) {
        printf("HID Keyboard: Time not synced, cannot generate TOTP\n");
        led_set_color(255, 0, 0); // Red
        sleep_ms(200);
        led_set_color(0, 255, 0); // Green
        return false;
    }
    
    // Get current timestamp
    uint64_t timestamp = time_sync_get_timestamp();
    
    // Generate TOTP code using libcotp
    // Note: This is a simplified version. In production, use the oath_protocol functions
    // For now, we'll use a placeholder
    
    // Simulate TOTP generation (replace with actual libcotp call)
    uint32_t code = (uint32_t)(timestamp / 30) % 1000000; // 6 digits
    
    printf("HID Keyboard: Generating code for '%s': %06d\n", name, code);
    
    // Type the code via HID
    // In a real implementation, this would send HID reports via TinyUSB
    // For now, we'll simulate by printing and blinking
    
    // Convert code to digits and "type" them
    char code_str[7];
    snprintf(code_str, sizeof(code_str), "%06d", code);
    
    // Blink LED for each digit
    for (int i = 0; i < 6; i++) {
        led_set_color(0, 255, 0); // Green
        sleep_ms(50);
        led_set_color(0, 0, 0); // Off
        sleep_ms(50);
    }
    
    // Simulate Enter key
    sleep_ms(100);
    led_set_color(0, 255, 0); // Green
    
    printf("HID Keyboard: Code typed: %s\n", code_str);
    
    return true;
}

/**
 * @brief Handle button press in HID mode
 */
void hid_keyboard_handle_button(void) {
    uint64_t now = time_us_64();
    
    // Debounce (500ms)
    if (now - last_button_press < 500000) {
        return;
    }
    
    last_button_press = now;
    
    // Check for long press (3 seconds) to exit HID mode
    // This would be handled in the main loop
    
    // Generate code
    hid_keyboard_generate_code();
}

/**
 * @brief Get current credential name
 */
const char* hid_keyboard_get_current_credential(void) {
    return oath_storage_list(current_credential_index);
}