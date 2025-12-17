#ifndef APPLET_MANAGER_H
#define APPLET_MANAGER_H

#include <stdbool.h>
#include <stdint.h>


#define MAX_AID_LEN 16

typedef void (*applet_handle_apdu_t)(uint8_t *apdu_in, uint16_t len_in,
                                     uint8_t *apdu_out, uint16_t *len_out);
typedef void (*applet_init_t)(void);

typedef struct {
  uint8_t aid[MAX_AID_LEN];
  uint8_t aid_len;
  applet_handle_apdu_t handle_apdu;
  applet_init_t init;
  const char *name;
} applet_t;

void applet_manager_init(void);
void applet_manager_handle_apdu(uint8_t *apdu_in, uint16_t len_in,
                                uint8_t *apdu_out, uint16_t *len_out);

#endif // APPLET_MANAGER_H
