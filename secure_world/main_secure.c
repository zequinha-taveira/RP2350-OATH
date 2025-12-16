#include "hardware/structs/sio.h"
#include "oath_protocol.h"
#include "pico/stdlib.h"
#include "secure_functions.h"
#include "security.h"
#include <stdint.h>
#include <stdio.h>


// Define the assumed location of Non-Secure Flash and System Control Block
// This matches memmap_ns.ld: 0x10040000
#define NON_SECURE_FLASH_OFFSET 0x10040000
#define VTOR_OFFSET (0xE000ED08)

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
  printf("Secure World: Security and OATH subsystems initialized.\n");

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
  while (1)
    ;
  return 0;
}
