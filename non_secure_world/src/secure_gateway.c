#include <stdio.h>
#include <string.h>
#include "secure_gateway.h"

//--------------------------------------------------------------------+
// Secure Gateway Implementation (Non-Secure World Side)
//--------------------------------------------------------------------+

// Placeholder for the Secure World entry point function pointer
// In a real TF-M environment, this would be a specific veneer call.
typedef bool (*secure_world_entry_t)(secure_gateway_func_id_t func_id, uint8_t *in_data, uint16_t in_len, uint8_t *out_data, uint16_t *out_len);

// NOTE: This is a simulation. In a real TrustZone setup, the Secure World
// would be a separate binary and the call would be handled by the TF-M core.
// For now, we simulate the call by directly calling the Secure World logic.
// The actual Secure World entry point is main_s in secure_world/src/main_s.c

// Forward declaration of the simulated Secure World handler
extern bool secure_world_handler(secure_gateway_func_id_t func_id, uint8_t *in_data, uint16_t in_len, uint8_t *out_data, uint16_t *out_len);

void secure_gateway_init(void) {
    printf("NS-Gateway: Initializing Secure World (SIMULATED JUMP)...\n");
    // In a real scenario, this would be a jump to the Secure World reset vector.
    // For now, we just print a message.
    
    // Simulate a call to the Secure World initialization function
    // secure_world_handler(SG_INIT, NULL, 0, NULL, NULL);
    printf("NS-Gateway: Secure World initialization complete.\n");
}

bool secure_gateway_oath_handle_apdu(uint8_t *apdu_in, uint16_t len_in, uint8_t *apdu_out, uint16_t *len_out) {
    printf("NS-Gateway: Calling Secure World for APDU handling...\n");
    
    // Simulate the call to the Secure World
    // In a real scenario, the data would be copied to a shared memory region
    // and the Secure World would be notified.
    
    // The Secure World handler is responsible for calling the oath_handle_apdu function.
    return secure_world_handler(SG_OATH_HANDLE_APDU, apdu_in, len_in, apdu_out, len_out);
}
