#ifndef _APDU_PROTOCOL_H_
#define _APDU_PROTOCOL_H_

#include <stdbool.h>
#include <stdint.h>

//--------------------------------------------------------------------+
// ISO 7816-4 APDU Constants
//--------------------------------------------------------------------+

// APDU Command structure (CLA INS P1 P2 Lc Data Le)
#define APDU_CLA_POS 0
#define APDU_INS_POS 1
#define APDU_P1_POS 2
#define APDU_P2_POS 3
#define APDU_LC_POS 4
#define APDU_DATA_POS 5

// Status Words (SW1 SW2)
#define SW_OK 0x9000
#define SW_FILE_NOT_FOUND 0x6A82
#define SW_WRONG_LENGTH 0x6700
#define SW_WRONG_DATA 0x6A80 // Added for invalid data parameter
#define SW_SECURITY_STATUS_NOT_SAT 0x6982
#define SW_CONDITIONS_NOT_SATISFIED 0x6985 // Conditions of use not satisfied
#define SW_INS_NOT_SUPPORTED 0x6D00
#define SW_CLA_NOT_SUPPORTED 0x6E00
#define SW_MEMORY_FAILURE 0x6581 // Added for memory write failure
#define SW_UNKNOWN 0x6F00        // General error

// APDU Instructions
#define INS_SELECT 0xA4
#define INS_CALCULATE 0xA1 // Used for CALCULATE, LIST, etc. in YKOATH
#define INS_VERSION 0x06   // Get Version
#define INS_VALIDATE 0xA3  // Validate PIN (YKOATH standard)

//--------------------------------------------------------------------+
// Yubico OATH Application Constants
//--------------------------------------------------------------------+

// Application Identifier (AID) for Yubico OATH
// A0 00 00 05 27 20 01
#define OATH_AID_LEN 7
static const uint8_t OATH_AID[OATH_AID_LEN] = {0xA0, 0x00, 0x00, 0x05,
                                               0x27, 0x20, 0x01};

// Application Identifier (AID) for Yubico Management
#define MGMT_AID_LEN 8
static const uint8_t MGMT_AID[MGMT_AID_LEN] = {0xA0, 0x00, 0x00, 0x05,
                                               0x27, 0x47, 0x11, 0x17};

// OATH Command Tags (used in the data field of INS_CALCULATE)
#define OATH_TAG_NAME 0x71
#define OATH_TAG_CHALLENGE 0x74
#define OATH_TAG_RESPONSE 0x75
#define OATH_TAG_PERIOD 0x76
#define OATH_TAG_ALGORITHM 0x77
#define OATH_TAG_CREDENTIAL_LIST 0x79

//--------------------------------------------------------------------+
// APDU Structure
//--------------------------------------------------------------------+

typedef struct {
  uint8_t cla;
  uint8_t ins;
  uint8_t p1;
  uint8_t p2;
  uint16_t lc; // Length of data field
  uint8_t *data;
  uint16_t le; // Expected length of response
} apdu_command_t;

#endif // _APDU_PROTOCOL_H_
