#include "tusb.h"
#include "pico/stdlib.h"
#include "ccid_device.h" // Custom header for CCID definitions
#include "ccid_protocol.h" // CCID message structures and constants

//--------------------------------------------------------------------+
// CCID Device Implementation
//--------------------------------------------------------------------+

// Global buffer for incoming CCID messages
uint8_t ccid_rx_buffer[CFG_TUD_CCID_RX_BUFSIZE];

// Global buffer for outgoing CCID messages
uint8_t ccid_tx_buffer[CFG_TUD_CCID_TX_BUFSIZE];

// Custom CCID device driver structure (Placeholder)
// This structure will hold the state of the emulated smart card
typedef struct {
    bool is_card_present;
    uint8_t atr[32]; // Answer To Reset
    // Add more state variables here
} ccid_state_t;

static ccid_state_t ccid_state = {
    .is_card_present = true, // Always present for emulated token
    .atr = {0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
};

// Forward declaration for APDU handler
extern void oath_handle_apdu(uint8_t *apdu_in, uint16_t len_in, uint8_t *apdu_out, uint16_t *len_out);

// Forward declaration for CCID message handler
void ccid_message_handler(uint8_t const *msg, uint32_t len);

//--------------------------------------------------------------------+
// TinyUSB Callbacks (Custom CCID Class)
//--------------------------------------------------------------------+

// Invoked when a CCID message is received from the host
void tud_ccid_rx_cb(uint8_t rhport, uint8_t *buffer, uint32_t len) {
    // Pass the message to the main handler
    ccid_message_handler(buffer, len);
}

//--------------------------------------------------------------------+
// CCID Message Handler
//--------------------------------------------------------------------+

void ccid_message_handler(uint8_t const *msg, uint32_t len) {
    ccid_msg_header_t const *header = (ccid_msg_header_t const *)msg;
    uint8_t rhport = 0; // Only one port for now

    if (len < CCID_HEADER_SIZE) {
        printf("CCID: ERROR - Message too short (%lu bytes)\n", len);
        return;
    }

    printf("CCID: RX Type=0x%02X, Len=%lu, Slot=%u, Seq=%u\n", 
           header->bMessageType, header->dwLength, header->bSlot, header->bSeq);

    switch (header->bMessageType) {
        case PC_TO_RDR_ICCPOWERON:
            // Respond with SlotStatus (ICC Present)
            rdr_to_pc_slotstatus_t poweron_resp = {
                .bMessageType = RDR_TO_PC_SLOTSTATUS,
                .dwLength = 0,
                .bSlot = header->bSlot,
                .bSeq = header->bSeq,
                .bStatus = SLOT_STATUS_ICC_PRESENT,
                .bError = 0,
                .bSpecific = 0
            };
            tud_ccid_tx(rhport, (uint8_t const *)&poweron_resp, sizeof(poweron_resp));
            break;

        case PC_TO_RDR_XFRBLOCK:
        {
            // This is where the APDU is received.
            uint8_t const *apdu_command = msg + sizeof(ccid_msg_header_t);
            uint16_t apdu_len = header->dwLength;

            uint8_t apdu_response[MAX_APDU_SIZE + 2]; // Max APDU response size + SW1/SW2
            uint16_t response_len = 0;

            // Call OATH handler (Secure Gateway Call in TrustZone architecture)
            oath_handle_apdu(apdu_command, apdu_len, apdu_response, &response_len);

            // Format RDR_to_PC_DataBlock with APDU response
            rdr_to_pc_datablock_t xfr_resp = {
                .bMessageType = RDR_TO_PC_DATABLOCK,
                .dwLength = response_len,
                .bSlot = header->bSlot,
                .bSeq = header->bSeq,
                .bStatus = SLOT_STATUS_ICC_PRESENT,
                .bError = 0,
                .bChainParameter = 0
            };
            
            // Copy APDU response data
            memcpy(xfr_resp.abData, apdu_response, response_len);
            
            // Calculate total response size: Header + Data Length
            size_t total_len = sizeof(ccid_msg_header_t) + 3 + response_len; // 10-byte header + 3-byte specific + data

            tud_ccid_tx(rhport, (uint8_t const *)&xfr_resp, total_len);
            break;
        }

        case PC_TO_RDR_ICCPOWEROFF:
            // Respond with SlotStatus (ICC Absent)
            rdr_to_pc_slotstatus_t poweroff_resp = {
                .bMessageType = RDR_TO_PC_SLOTSTATUS,
                .dwLength = 0,
                .bSlot = header->bSlot,
                .bSeq = header->bSeq,
                .bStatus = SLOT_STATUS_ICC_ABSENT,
                .bError = 0,
                .bSpecific = 0
            };
            tud_ccid_tx(rhport, (uint8_t const *)&poweroff_resp, sizeof(poweroff_resp));
            break;

        case PC_TO_RDR_GETSLOTSTATUS:
            // Respond with SlotStatus (ICC Present)
            rdr_to_pc_slotstatus_t status_resp = {
                .bMessageType = RDR_TO_PC_SLOTSTATUS,
                .dwLength = 0,
                .bSlot = header->bSlot,
                .bSeq = header->bSeq,
                .bStatus = SLOT_STATUS_ICC_PRESENT,
                .bError = 0,
                .bSpecific = 0
            };
            tud_ccid_tx(rhport, (uint8_t const *)&status_resp, sizeof(status_resp));
            break;

        default:
            printf("CCID: Unknown message type 0x%02X\n", header->bMessageType);
            // TODO: Send RDR_TO_PC_ERROR
            break;
    }
}

// Invoked when the host requests the CCID descriptor
uint16_t tud_ccid_get_descriptor_cb(uint8_t *buffer, uint16_t bufsize) {
    // TODO: Implement the full CCID descriptor structure here
    // This is a complex structure that defines the capabilities of the reader
    // For now, return 0 or a minimal descriptor
    return 0;
}

// Invoked when the host requests the ATR (Answer To Reset)
bool tud_ccid_get_atr_cb(uint8_t rhport, uint8_t *buffer, uint16_t *len) {
    // ATR is a fixed response for the emulated card
    *len = sizeof(ccid_state.atr);
    memcpy(buffer, ccid_state.atr, *len);
    return true;
}

// Invoked when the host requests slot status
bool tud_ccid_get_slot_status_cb(uint8_t rhport, uint8_t *buffer, uint16_t *len) {
    // TODO: Implement slot status response
    return true;
}

//--------------------------------------------------------------------+
// Custom CCID Task (Called from main.c)
//--------------------------------------------------------------------+

void ccid_task(void) {
    // This function can be used for any periodic CCID-related tasks,
    // such as handling interrupts or checking internal state.
}
