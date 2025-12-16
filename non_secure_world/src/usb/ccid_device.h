#ifndef _CCID_DEVICE_H_
#define _CCID_DEVICE_H_

#include <stdint.h>
#include <stdbool.h>

// Max size for APDU command/response
#define MAX_APDU_SIZE 256
#define CCID_HEADER_SIZE 10

// Function prototypes
void ccid_init(void);
void ccid_task(void);

#endif // _CCID_DEVICE_H_
