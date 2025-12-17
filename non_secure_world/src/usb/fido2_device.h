#ifndef _FIDO2_DEVICE_H_
#define _FIDO2_DEVICE_H_

#include <stdint.h>
#include <stdbool.h>

// FIDO2 / CTAP2 Constants
#define FIDO2_USAGE_PAGE         0xF1D0  // FIDO Alliance Usage Page
#define FIDO2_USAGE_CTAPHID      0x0001  // CTAPHID Usage
#define FIDO2_USAGE_DATA_IN      0x0020  // Data In
#define FIDO2_USAGE_DATA_OUT     0x0021  // Data Out

// CTAP HID Command Codes
#define CTAPHID_PING            0x01
#define CTAPHID_MSG             0x03
#define CTAPHID_LOCK            0x04
#define CTAPHID_INIT            0x06
#define CTAPHID_WINK            0x08
#define CTAPHID_CANCELL         0x11
#define CTAPHID_ERROR           0x3F
#define CTAPHID_KEEPALIVE       0x3B
#define CTAPHID_FRAME           0x80

// CTAP2 Command Codes
#define CTAP2_MAKE_CREDENTIAL   0x01
#define CTAP2_GET_ASSERTION     0x02
#define CTAP2_GET_INFO          0x04
#define CTAP2_CLIENT_PIN        0x06
#define CTAP2_RESET             0x07
#define CTAP2_GET_NEXT_ASSERTION 0x08
#define CTAP2_BIO_ENROLL        0x09
#define CTAP2_CREDENTIAL_MGMT   0x0A
#define CTAP2_SELECTION         0x0B
#define CTAP2_BIO_INFO          0x0C
#define CTAP2_CONFIG            0x0D
#define CTAP2_VENDOR_FIRST      0x40

// CTAP Status Codes
#define CTAP1_ERR_SUCCESS              0x00
#define CTAP1_ERR_INVALID_COMMAND      0x01
#define CTAP1_ERR_INVALID_PARAMETER    0x02
#define CTAP1_ERR_INVALID_LENGTH       0x03
#define CTAP1_ERR_INVALID_SEQUENCE     0x04
#define CTAP1_ERR_TIMEOUT              0x05
#define CTAP1_ERR_CHANNEL_BUSY         0x06
#define CTAP2_ERR_CBOR_PARSING         0x10
#define CTAP2_ERR_CBOR_UNEXPECTED_TYPE 0x11
#define CTAP2_ERR_CBOR_INVALID_VALUE   0x12
#define CTAP2_ERR_CBOR_UNKNOWN         0x13
#define CTAP2_ERR_CBOR_MAP_KEY         0x14
#define CTAP2_ERR_CBOR_LIMIT_EXCEEDED  0x15
#define CTAP2_ERR_CBOR_INVALID         0x16
#define CTAP2_ERR_UNSUPPORTED_OPTION   0x20
#define CTAP2_ERR_UNSUPPORTED_ALGORITHM 0x21
#define CTAP2_ERR_CREDENTIAL_EXCLUDED  0x22
#define CTAP2_ERR_PROCESSING           0x23
#define CTAP2_ERR_INVALID_CREDENTIAL   0x24
#define CTAP2_ERR_USER_ACTION_PENDING  0x25
#define CTAP2_ERR_OPERATION_PENDING    0x26
#define CTAP2_ERR_NO_OPERATIONS        0x27
#define CTAP2_ERR_AUTHENTICATOR_LOCKED 0x28
#define CTAP2_ERR_NO_CREDENTIALS       0x29
#define CTAP2_ERR_USER_ACTION_TIMEOUT  0x2A
#define CTAP2_ERR_NOT_ALLOWED          0x2B
#define CTAP2_ERR_PIN_INVALID          0x2C
#define CTAP2_ERR_PIN_BLOCKED          0x2D
#define CTAP2_ERR_PIN_AUTH_INVALID     0x2E
#define CTAP2_ERR_PIN_AUTH_BLOCKED     0x2F
#define CTAP2_ERR_PIN_REQUIRED         0x30
#define CTAP2_ERR_PIN_POLICY_VIOLATION 0x31
#define CTAP2_ERR_PIN_TOKEN_EXPIRED    0x32
#define CTAP2_ERR_REQUEST_TOO_LARGE    0x33
#define CTAP2_ERR_ACTION_TIMEOUT       0x34
#define CTAP2_ERR_UP_REQUIRED          0x35

// Keepalive Status
#define CTAP2_KEEPALIVE_STATUS_PROCESSING 0x01
#define CTAP2_KEEPALIVE_STATUS_UPNEEDED   0x02

// FIDO2 HID Report Size
#define FIDO2_REPORT_SIZE       64
#define FIDO2_INIT_DATA_SIZE    8

// Channel IDs
#define FIDO2_BROADCAST_CHANNEL 0xFFFFFFFF
#define FIDO2_CHANNEL_FIRST     0x00000001
#define FIDO2_CHANNEL_LAST      0xFFFFFFFF

// FIDO2 Device State
typedef struct {
    bool initialized;
    bool connected;
    uint8_t current_channel;
    uint8_t nonce[8]; // Init nonce
    uint8_t pin_token[32];
    uint8_t pin_protocol;
    uint16_t max_msg_size;
} fido2_state_t;

// CTAPHID Frame Structure
typedef struct __attribute__((packed)) {
    uint8_t cmd;
    uint8_t bcnth;
    uint8_t bcntl;
    uint8_t data[FIDO2_REPORT_SIZE - 3];
} ctaphid_frame_t;

// CTAPHID Init Response
typedef struct __attribute__((packed)) {
    uint8_t cmd;
    uint8_t bcnth;
    uint8_t bcntl;
    uint8_t nonce[8];
    uint8_t cid[4];
    uint8_t version;
    uint8_t capabilities;
    uint8_t reserved[7];
} ctaphid_init_response_t;

// Function prototypes
void fido2_init(void);
void fido2_task(void);
void fido2_handle_report(uint8_t const *report, uint32_t len);
bool fido2_send_report(uint8_t const *report, uint32_t len);
void fido2_send_keepalive(uint8_t status);
void fido2_send_error(uint8_t error_code);

// CTAP2 Command Handlers
void fido2_handle_init(ctaphid_frame_t const *frame);
void fido2_handle_ping(ctaphid_frame_t const *frame);
void fido2_handle_msg(ctaphid_frame_t const *frame);
void fido2_handle_cancel(void);
void fido2_handle_wink(void);

// CTAP2 Application Command Handlers
void fido2_handle_make_credential(uint8_t const *data, uint16_t len);
void fido2_handle_get_assertion(uint8_t const *data, uint16_t len);
void fido2_handle_get_info(void);
void fido2_handle_client_pin(uint8_t const *data, uint16_t len);
void fido2_handle_reset(void);

// CBOR Helper Functions
uint8_t* cbor_encode_map(uint8_t *buffer, uint8_t size);
uint8_t* cbor_encode_uint(uint8_t *buffer, uint64_t value);
uint8_t* cbor_encode_string(uint8_t *buffer, const char *str);
uint8_t* cbor_encode_bytes(uint8_t *buffer, const uint8_t *data, uint16_t len);
uint8_t* cbor_encode_array(uint8_t *buffer, uint8_t size);

// Credential Storage
bool fido2_store_credential(uint8_t *credential_id, uint8_t *credential_data, uint16_t data_len);
bool fido2_retrieve_credential(uint8_t *credential_id, uint8_t *credential_data, uint16_t *data_len);
bool fido2_delete_credential(uint8_t *credential_id);

// User Verification
bool fido2_verify_user(void);
bool fido2_verify_pin(uint8_t *pin, uint8_t pin_len);
bool fido2_set_pin(uint8_t *pin, uint8_t pin_len);

#endif // _FIDO2_DEVICE_H_