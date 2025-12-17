#ifndef _FIDO2_CTAP21_H_
#define _FIDO2_CTAP21_H_

#include <stdbool.h>
#include <stdint.h>

// CTAP2.1 Extended Commands
#define CTAP21_GET_CREDENTIALS 0x0A // credentialManagement
#define CTAP21_GET_NEXT_ASSERTION 0x08
#define CTAP21_SELECTION 0x0B
#define CTAP21_BIO_INFO 0x0C
#define CTAP21_CONFIG 0x0D

// CTAP2.1 Options
#define OPTION_RESIDENT_KEY "rk"
#define OPTION_USER_VERIFICATION "uv"
#define OPTION_USER_PRESENCE "up"
#define OPTION_PLAT "plat"
#define OPTION_CLIENT_PIN "clientPin"
#define OPTION_RESET_LATENCY "resetLatency"
#define OPTION_ENTERPRISE "enterprise"

// CTAP2.1 Resident Key Management
#define RK_CMD_GET_CREDS 0x01
#define RK_CMD_DELETE_CRED 0x02
#define RK_CMD_UPDATE_USER 0x03
#define RK_CMD_GET_CREDS_METADATA 0x04

// CTAP2.1 Config Commands
#define CONFIG_CMD_ENABLE_ENTERPRISE 0x01
#define CONFIG_CMD_TOGGLE_ALWAYS_UV 0x02
#define CONFIG_CMD_SET_MIN_PIN_LENGTH 0x03
#define CONFIG_CMD_VENDOR_PROTOTYPE 0x04

// CTAP2.1 Selection Commands
#define SELECTION_CMD_GET 0x00
#define SELECTION_CMD_SELECT 0x01

// CTAP2.1 Bio Enrollment Commands
#define BIO_CMD_GET_INFO 0x00
#define BIO_CMD_ENROLL 0x01
#define BIO_CMD_ENUMERATE 0x02
#define BIO_CMD_REMOVE 0x03
#define BIO_CMD_SET_NAME 0x04

// Bio Modality
#define MODALITY_FINGERPRINT 0x01

// Bio Sensor Type
#define SENSOR_TYPE_TOUCH 0x01

// Bio Limits
#define MAX_FINGERPRINTS 10

// Bio Capabilities
#define CAPABILITY_TIMEOUT 0x01
#define CAPABILITY_USER_VER 0x02
#define CAPABILITY_ENROLL 0x04

// CTAP2.1 Status Codes (Extended)
#define CTAP21_ERR_INVALID_COMMAND 0x01
#define CTAP21_ERR_INVALID_PARAMETER 0x02
#define CTAP21_ERR_INVALID_LENGTH 0x03
#define CTAP21_ERR_CREDENTIAL_EXCLUDED 0x22
#define CTAP21_ERR_PROCESSING 0x23
#define CTAP21_ERR_INVALID_CREDENTIAL 0x24
#define CTAP21_ERR_USER_ACTION_PENDING 0x25
#define CTAP21_ERR_OPERATION_PENDING 0x26
#define CTAP21_ERR_NO_OPERATIONS 0x27
#define CTAP21_ERR_AUTHENTICATOR_LOCKED 0x28
#define CTAP21_ERR_NO_CREDENTIALS 0x29
#define CTAP21_ERR_USER_ACTION_TIMEOUT 0x2A
#define CTAP21_ERR_NOT_ALLOWED 0x2B
#define CTAP21_ERR_PIN_INVALID 0x2C
#define CTAP21_ERR_PIN_BLOCKED 0x2D
#define CTAP21_ERR_PIN_AUTH_INVALID 0x2E
#define CTAP21_ERR_PIN_AUTH_BLOCKED 0x2F
#define CTAP21_ERR_PIN_REQUIRED 0x30
#define CTAP21_ERR_PIN_POLICY_VIOLATION 0x31
#define CTAP21_ERR_PIN_TOKEN_EXPIRED 0x32
#define CTAP21_ERR_REQUEST_TOO_LARGE 0x33
#define CTAP21_ERR_ACTION_TIMEOUT 0x34
#define CTAP21_ERR_UP_REQUIRED 0x35

// CTAP2.1 Extended Options
typedef struct __attribute__((packed)) {
  bool rk;            // Resident Key
  bool uv;            // User Verification
  bool up;            // User Presence
  bool plat;          // Platform
  bool client_pin;    // Client PIN
  bool reset_latency; // Reset Latency
  bool enterprise;    // Enterprise Attestation
} ctap21_options_t;

// Resident Key Credential
typedef struct __attribute__((packed)) {
  uint8_t credential_id[32];
  uint8_t public_key[65];
  uint8_t user_id[32];
  uint8_t user_name[64];
  uint8_t display_name[64];
  uint8_t rp_id[64];
  uint8_t timestamp[8];
  uint8_t use_count[4];
} resident_key_t;

// Credential Management Response
typedef struct __attribute__((packed)) {
  uint8_t status;
  uint8_t credential_count;
  resident_key_t credentials[10];
} credential_mgmt_resp_t;

// CTAP2.1 Config Request
typedef struct __attribute__((packed)) {
  uint8_t command;
  uint8_t subcommand;
  uint8_t param1;
  uint8_t param2;
  uint8_t data_len;
  uint8_t data[256];
} ctap21_config_req_t;

// CTAP2.1 Config Response
typedef struct __attribute__((packed)) {
  uint8_t status;
  uint8_t data_len;
  uint8_t data[256];
} ctap21_config_resp_t;

// CTAP2.1 Get Next Assertion
typedef struct __attribute__((packed)) {
  uint8_t command;
  uint8_t credential_index;
} ctap21_get_next_req_t;

// CTAP2.1 Selection Request
typedef struct __attribute__((packed)) {
  uint8_t command;
  uint8_t action;
  uint8_t timeout;
} ctap21_selection_req_t;

// CTAP2.1 Bio Info
typedef struct __attribute__((packed)) {
  uint8_t modality;
  uint8_t fingerprint_type;
  uint8_t max_fingerprints;
  uint8_t current_fingerprints;
  uint8_t capabilities;
} ctap21_bio_info_t;

// Function prototypes
void fido2_ctap21_init(void);
void fido2_ctap21_handle_command(uint8_t command, uint8_t const *data,
                                 uint16_t len);

// Credential Management
bool ctap21_get_credentials(uint8_t *rp_id, uint8_t rp_len,
                            credential_mgmt_resp_t *resp);
bool ctap21_delete_credential(uint8_t *credential_id);
bool ctap21_update_user(uint8_t *credential_id, uint8_t *user_name,
                        uint8_t name_len);
bool ctap21_get_credentials_metadata(uint8_t *rp_id, uint8_t rp_len,
                                     uint8_t *count);

// Configuration
bool ctap21_enable_enterprise(void);
bool ctap21_toggle_always_uv(bool enable);
bool ctap21_set_min_pin_length(uint8_t length);
bool ctap21_vendor_prototype(uint8_t *data, uint8_t len);

// Selection
bool ctap21_selection_get(uint8_t timeout);
bool ctap21_selection_select(uint8_t index);

// Get Next Assertion
bool ctap21_get_next_assertion(uint8_t index, uint8_t *assertion,
                               uint8_t *assertion_len);

// Bio Info
bool ctap21_bio_get_info(ctap21_bio_info_t *info);

// Extended Options
bool ctap21_get_options(ctap21_options_t *options);
bool ctap21_set_options(ctap21_options_t *options);

// Pin Policy
bool ctap21_validate_pin_policy(uint8_t *pin, uint8_t pin_len);

// Enterprise Attestation
bool ctap21_enterprise_attest(uint8_t *client_data, uint8_t data_len,
                              uint8_t *attestation, uint8_t *att_len);

// Resident Key Operations
bool ctap21_store_resident_key(resident_key_t *rk);
bool ctap21_retrieve_resident_key(uint8_t *credential_id, resident_key_t *rk);
bool ctap21_list_resident_keys(credential_mgmt_resp_t *resp);

// CTAP2.1 Specific Helpers
uint8_t ctap21_map_to_ctap2_error(uint8_t internal_error);
bool ctap21_validate_request_size(uint16_t size, uint16_t max_size);
bool ctap21_check_always_uv(void);
bool ctap21_check_pin_token_expired(void);

// External Bio Helpers
uint8_t get_fingerprint_count(void);

#endif // _FIDO2_CTAP21_H_