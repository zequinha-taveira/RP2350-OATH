#ifndef _CCID_PROTOCOL_H_
#define _CCID_PROTOCOL_H_

#include <stdbool.h>
#include <stdint.h>


//--------------------------------------------------------------------+
// CCID Constants
//--------------------------------------------------------------------+

// Message Types (PC to Reader)
#define PC_TO_RDR_ICCPOWERON 0x62
#define PC_TO_RDR_XFRBLOCK 0x6F
#define PC_TO_RDR_ICCPOWEROFF 0x63
#define PC_TO_RDR_GETSLOTSTATUS 0x65

// Message Types (Reader to PC)
#define RDR_TO_PC_DATABLOCK 0x80
#define RDR_TO_PC_SLOTSTATUS 0x81
#define RDR_TO_PC_ICCSTATUS 0x82 // Not used in this simple implementation

// Slot Status
#define SLOT_STATUS_ICC_PRESENT 0x00
#define SLOT_STATUS_ICC_ABSENT 0x01
#define SLOT_STATUS_ICC_INACTIVE 0x02

//--------------------------------------------------------------------+
// CCID Message Structures (Simplified)
//--------------------------------------------------------------------+

// Base structure for all PC_to_RDR messages (10 bytes header)
typedef struct __attribute__((packed)) {
  uint8_t bMessageType;
  uint32_t dwLength;
  uint8_t bSlot;
  uint8_t bSeq;
  uint8_t bSpecific_0;
  uint8_t bSpecific_1;
  uint8_t bSpecific_2;
  uint8_t bSpecific_3;
} ccid_msg_header_t;

// PC_to_RDR_XfrBlock (0x6F)
typedef struct __attribute__((packed)) {
  ccid_msg_header_t header;
  uint8_t abData[256]; // Max APDU size
} pc_to_rdr_xfrblock_t;

// RDR_to_PC_DataBlock (0x80)
typedef struct __attribute__((packed)) {
  uint8_t bMessageType;
  uint32_t dwLength;
  uint8_t bSlot;
  uint8_t bSeq;
  uint8_t bStatus;
  uint8_t bError;
  uint8_t bChainParameter;
  uint8_t abData[256]; // Max APDU response size
} rdr_to_pc_datablock_t;

// RDR_to_PC_SlotStatus (0x81)
typedef struct __attribute__((packed)) {
  uint8_t bMessageType;
  uint32_t dwLength; // Should be 0
  uint8_t bSlot;
  uint8_t bSeq;
  uint8_t bStatus;
  uint8_t bError;
  uint8_t bSpecific;
} rdr_to_pc_slotstatus_t;

//--------------------------------------------------------------------+
// Function Prototypes
//--------------------------------------------------------------------+

/**
 * @brief Main handler for incoming CCID messages from the host.
 *
 * @param msg Pointer to the raw CCID message buffer.
 * @param len Length of the message.
 */
void ccid_message_handler(uint8_t const *msg, uint32_t len);

#endif // _CCID_PROTOCOL_H_
