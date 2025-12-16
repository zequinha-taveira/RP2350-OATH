#include <stdio.h>
#include <string.h>
#include "secure_gateway.h" // Shared header for function IDs
#include "oath_protocol.h"  // Secure World OATH logic

//--------------------------------------------------------------------+
// Secure Gateway Implementation (Secure World Side)
//--------------------------------------------------------------------+

// Secure World Handler - The single entry point for Non-Secure calls
bool secure_world_handler(secure_gateway_func_id_t func_id, uint8_t *in_data, uint16_t in_len, uint8_t *out_data, uint16_t *out_len) {
    printf("Secure World: Received call 0x%02X from Non-Secure World.\n", func_id);
    
    switch (func_id) {
        case SG_INIT:
            // Initialization is handled by main_s, but we can use this for post-boot setup
            printf("Secure World: SG_INIT called.\n");
            return true;
            
        case SG_OATH_HANDLE_APDU:
            // Call the core OATH logic
            oath_handle_apdu(in_data, in_len, out_data, out_len);
            return true;
            
        case SG_GET_TIME:
            // TODO: Implement secure time retrieval
            printf("Secure World: SG_GET_TIME called (TODO).\n");
            return false;
            
        default:
            printf("Secure World: Unknown function ID 0x%02X.\n", func_id);
            return false;
    }
}
