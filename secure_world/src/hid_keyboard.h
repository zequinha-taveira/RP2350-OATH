#ifndef HID_KEYBOARD_H
#define HID_KEYBOARD_H

#include <stdbool.h>

/**
 * @brief Initialize HID Keyboard mode
 */
void hid_keyboard_init(void);

/**
 * @brief Enable/disable HID Keyboard mode
 * @param enabled true to enable, false to disable
 */
void hid_keyboard_set_mode(bool enabled);

/**
 * @brief Get current HID mode
 * @return true if HID mode is enabled
 */
bool hid_keyboard_get_mode(void);

/**
 * @brief Cycle to next credential
 */
void hid_keyboard_next_credential(void);

/**
 * @brief Generate and type TOTP code for current credential
 * @return true if successful, false otherwise
 */
bool hid_keyboard_generate_code(void);

/**
 * @brief Handle button press in HID mode
 */
void hid_keyboard_handle_button(void);

/**
 * @brief Get current credential name
 * @return Pointer to credential name or NULL
 */
const char* hid_keyboard_get_current_credential(void);

#endif // HID_KEYBOARD_H