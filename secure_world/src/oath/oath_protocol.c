#include "oath_protocol.h"
#include "pico/stdlib.h"
#include "oath_storage.h"
#include "apdu_protocol.h" // For APDU and OATH constants
#include "crypto/hmac.h"   // For HMAC calculation
#include "libcotp/src/cotp.h" // Using libcotp for core logic

//--------------------------------------------------------------------+
// OATH Protocol Implementation (Secure World Logic Placeholder)
//--------------------------------------------------------------------+

// Global state for the OATH application
static bool oath_app_selected = false;

// Helper function to send a status word (SW) response
static void send_sw(uint16_t sw, uint8_t *apdu_out, uint16_t *len_out) {
    apdu_out[0] = (uint8_t)(sw >> 8);
    apdu_out[1] = (uint8_t)(sw & 0xFF);
    *len_out = 2;
}

// Handler for the ISO 7816 SELECT command (INS=A4)
static void handle_select_apdu(uint8_t *apdu_in, uint16_t len_in, uint8_t *apdu_out, uint16_t *len_out) {
    // APDU: CLA=00, INS=A4, P1=04 (Select by AID), P2=00 (First or only occurrence)
    // Lc is the length of the AID (7 bytes)
    
    if (apdu_in[APDU_P1_POS] != 0x04 || apdu_in[APDU_P2_POS] != 0x00) {
        send_sw(SW_WRONG_LENGTH, apdu_out, len_out);
        return;
    }

    uint8_t lc = apdu_in[APDU_LC_POS];
    uint8_t *aid = apdu_in + APDU_DATA_POS;

    if (lc == OATH_AID_LEN && memcmp(aid, OATH_AID, OATH_AID_LEN) == 0) {
        oath_app_selected = true;
        printf("OATH: Application selected successfully.\n");
        send_sw(SW_OK, apdu_out, len_out);
    } else {
        oath_app_selected = false;
        printf("OATH: Application selection failed.\n");
        send_sw(SW_FILE_NOT_FOUND, apdu_out, len_out);
    }
}

// Handler for the Yubico OATH CALCULATE/LIST command (INS=A1)
static void handle_oath_command(uint8_t *apdu_in, uint16_t len_in, uint8_t *apdu_out, uint16_t *len_out) {
    // Yubico OATH uses INS=A1 for multiple commands, distinguished by tags in the data field.
    // The most common are CALCULATE (0x74) and LIST (0x79).
    
    if (!oath_app_selected) {
        send_sw(SW_SECURITY_STATUS_NOT_SAT, apdu_out, len_out);
        return;
    }

    // The data field starts with a tag (e.g., 0x74 for CALCULATE)
    uint8_t tag = apdu_in[APDU_DATA_POS];
    
    switch (tag) {
        case OATH_TAG_CHALLENGE: // 0x74 - CALCULATE
            printf("OATH: Handling CALCULATE command...\n");
            // TODO: Implement full CALCULATE logic (load secret, get time, calculate TOTP/HOTP)
            
            // Placeholder: Return success (SW_OK) for now
            send_sw(SW_OK, apdu_out, len_out);
            break;
            
        case OATH_TAG_CREDENTIAL_LIST: // 0x79 - LIST
            printf("OATH: Handling LIST command...\n");
            // TODO: Implement LIST logic (read index from flash, return list of names)
            
            // Placeholder: Return success (SW_OK) for now
            send_sw(SW_OK, apdu_out, len_out);
            break;
            
        default:
            printf("OATH: Unknown OATH command tag 0x%02X\n", tag);
            send_sw(SW_INS_NOT_SUPPORTED, apdu_out, len_out);
            break;
    }
}

// Placeholder for OATH protocol initialization
void oath_init(void) {
    printf("OATH: Initializing protocol handler...\n");
    // Load OATH application state (e.g., selected application)
    oath_app_selected = false;
    // TODO: Load credential index from flash
    oath_storage_init();
}

// Main APDU handling function (called from CCID driver)
void oath_handle_apdu(uint8_t *apdu_in, uint16_t len_in, uint8_t *apdu_out, uint16_t *len_out) {
    // APDU structure: CLA | INS | P1 | P2 | Lc | Data | Le
    
    if (len_in < 5) {
        send_sw(SW_WRONG_LENGTH, apdu_out, len_out);
        return;
    }
    
    uint8_t cla = apdu_in[APDU_CLA_POS];
    uint8_t ins = apdu_in[APDU_INS_POS];

    if (cla != 0x00) {
        send_sw(SW_CLA_NOT_SUPPORTED, apdu_out, len_out);
        return;
    }

    switch (ins) {
        case INS_SELECT:
            handle_select_apdu(apdu_in, len_in, apdu_out, len_out);
            break;
            
        case INS_CALCULATE: // INS=A1 in Yubico OATH is mapped to INS_CALCULATE in apdu_protocol.h
            handle_oath_command(apdu_in, len_in, apdu_out, len_out);
            break;

        default:
            send_sw(SW_INS_NOT_SUPPORTED, apdu_out, len_out);
            break;
    }
}
