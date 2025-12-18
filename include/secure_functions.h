#ifndef SECURE_FUNCTIONS_H
#define SECURE_FUNCTIONS_H

#include "secure_gateway.h" // For secure_gateway_func_id_t
#include <stdint.h>

#ifdef PICO_TRUSTZONE_SECURE_BUILD
// In Secure World, we define these with the CMSE entry attribute
#if !defined(__cmse_nonsecure_entry)
#define __cmse_nonsecure_entry __attribute__((cmse_nonsecure_entry))
#endif
#else
// In Non-Secure World, these are just normal function prototypes
#define __cmse_nonsecure_entry
#endif

// Secure World Handler - The single entry point for Non-Secure calls
// Returns: Positive value (bytes written) on success, Negative value on error.
int32_t __cmse_nonsecure_entry
secure_world_handler(secure_gateway_func_id_t func_id, uint8_t *in_data,
                     uint16_t in_len, uint8_t *out_data, uint16_t out_max_len);

#endif // SECURE_FUNCTIONS_H
