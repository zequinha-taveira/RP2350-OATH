#ifndef APPLET_MANAGER_H
#define APPLET_MANAGER_H

#include <stdbool.h>
#include <stdint.h>


/**
 * @brief Structure representing a secure world applet.
 */
typedef struct {
  const uint8_t *aid;
  uint8_t aid_len;
  void (*init)(void);
  void (*handle_apdu)(uint8_t *apdu_in, uint16_t len_in, uint8_t *apdu_out,
                      uint16_t *len_out);
} secure_applet_t;

/**
 * @brief Initializes the applet manager and registers all known applets.
 */
void applet_manager_init(void);

/**
 * @brief Handles an incoming APDU by routing it to the appropriate applet.
 *
 * @param apdu_in Input buffer containing the raw APDU.
 * @param len_in Length of the input APDU.
 * @param apdu_out Output buffer for the response.
 * @param len_out Pointer to store the length of the response.
 */
void applet_manager_process_apdu(uint8_t *apdu_in, uint16_t len_in,
                                 uint8_t *apdu_out, uint16_t *len_out);

#endif // APPLET_MANAGER_H
