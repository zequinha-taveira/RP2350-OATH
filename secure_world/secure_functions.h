#ifndef SECURE_FUNCTIONS_H
#define SECURE_FUNCTIONS_H

#include <stdint.h>

/*
 * This header defines the interface between Secure and Non-Secure worlds.
 * It should be included by both the Secure World implementation
 * and the Non-Secure World application.
 */

#ifdef PICO_TRUSTZONE_SECURE_BUILD
// In Secure World, we define these with the CMSE entry attribute
#if !defined(__cmse_nonsecure_entry)
#define __cmse_nonsecure_entry __attribute__((cmse_nonsecure_entry))
#endif
#else
// In Non-Secure World, these are just normal function prototypes
#define __cmse_nonsecure_entry
#endif

// Example NSC Function: Adds two numbers securely
// This is a placeholder for actual OATH/Crypto logic
int __cmse_nonsecure_entry secure_add(int a, int b);

// Example NSC Function: Get Secure World Status
// Returns 1 if secure world is happy, 0 otherwise
int __cmse_nonsecure_entry secure_get_status(void);

#endif // SECURE_FUNCTIONS_H
