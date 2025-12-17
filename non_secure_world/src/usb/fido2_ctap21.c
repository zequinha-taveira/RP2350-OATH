#include "fido2_ctap21.h"
#include "fido2_device.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

// CTAP2.1 State
typedef struct {
    ctap21_options_t options;
    bool enterprise_enabled;
    bool always_uv;
    uint8_t min_pin_length;
    uint8_t pin_token[32];
    uint64_t pin_token_expiry;
    uint8_t credential_count;
    resident_key_t resident_keys[10];
} ctap21_state_t;

static ctap21_state_t ctap21_state = {
    .options = {
        .rk = true,
        .uv = false,
        .up = true,
        .plat = false,
        .client_pin = false,
        .reset_latency = false,
        .enterprise = false
    },
    .enterprise_enabled = false,
    .always_uv = false,
    .min_pin_length = 6,
    .pin_token_expiry = 0,
    .credential_count = 0
};

void fido2_ctap21_init(void) {
    printf("FIDO2 CTAP2.1: Initialized\n");
    memset(&ctap21_state, 0, sizeof(ctap21_state));
    
    // Set default options
    ctap21_state.options.rk = true;
    ctap21_state.options.up = true;
    ctap21_state.min_pin_length = 6;
}

void fido2_ctap21_handle_command(uint8_t command, uint8_t const *data, uint16_t len) {
    printf("FIDO2 CTAP2.1: Command 0x%02X received\n", command);
    
    uint8_t response[512];
    uint16_t response_len = 0;
    
    switch (command) {
        case CTAP21_GET_CREDENTIALS:
            {
                uint8_t rp_len = data[0];
                uint8_t *rp_id = (uint8_t*)(data + 1);
                
                credential_mgmt_resp_t resp;
                if (ctap21_get_credentials(rp_id, rp_len, &resp)) {
                    response[0] = CTAP21_GET_CREDENTIALS;
                    response[1] = resp.status;
                    response[2] = resp.credential_count;
                    uint8_t *ptr = response + 3;
                    for (int i = 0; i < resp.credential_count; i++) {
                        memcpy(ptr, &resp.credentials[i], sizeof(resident_key_t));
                        ptr += sizeof(resident_key_t);
                    }
                    response_len = ptr - response;
                } else {
                    response[0] = CTAP21_GET_CREDENTIALS;
                    response[1] = CTAP21_ERR_NO_CREDENTIALS;
                    response_len = 2;
                }
            }
            break;
            
        case CTAP21_GET_NEXT_ASSERTION:
            {
                uint8_t index = data[0];
                uint8_t assertion[256];
                uint8_t assertion_len;
                
                if (ctap21_get_next_assertion(index, assertion, &assertion_len)) {
                    response[0] = CTAP21_GET_NEXT_ASSERTION;
                    response[1] = 0x00; // Success
                    memcpy(response + 2, assertion, assertion_len);
                    response_len = 2 + assertion_len;
                } else {
                    response[0] = CTAP21_GET_NEXT_ASSERTION;
                    response[1] = CTAP21_ERR_NO_OPERATIONS;
                    response_len = 2;
                }
            }
            break;
            
        case CTAP21_SELECTION:
            {
                ctap21_selection_req_t *req = (ctap21_selection_req_t*)data;
                if (req->action == SELECTION_CMD_GET) {
                    if (ctap21_selection_get(req->timeout)) {
                        response[0] = CTAP21_SELECTION;
                        response[1] = 0x00;
                        response_len = 2;
                    } else {
                        response[0] = CTAP21_SELECTION;
                        response[1] = CTAP21_ERR_USER_ACTION_TIMEOUT;
                        response_len = 2;
                    }
                } else if (req->action == SELECTION_CMD_SELECT) {
                    if (ctap21_selection_select(req->timeout)) {
                        response[0] = CTAP21_SELECTION;
                        response[1] = 0x00;
                        response_len = 2;
                    } else {
                        response[0] = CTAP21_SELECTION;
                        response[1] = CTAP21_ERR_INVALID_CREDENTIAL;
                        response_len = 2;
                    }
                }
            }
            break;
            
        case CTAP21_BIO_INFO:
            {
                ctap21_bio_info_t info;
                if (ctap21_bio_get_info(&info)) {
                    response[0] = CTAP21_BIO_INFO;
                    response[1] = 0x00;
                    memcpy(response + 2, &info, sizeof(ctap21_bio_info_t));
                    response_len = 2 + sizeof(ctap21_bio_info_t);
                } else {
                    response[0] = CTAP21_BIO_INFO;
                    response[1] = CTAP21_ERR_NOT_ALLOWED;
                    response_len = 2;
                }
            }
            break;
            
        case CTAP21_CONFIG:
            {
                ctap21_config_req_t *req = (ctap21_config_req_t*)data;
                ctap21_config_resp_t resp;
                
                switch (req->subcommand) {
                    case CONFIG_CMD_ENABLE_ENTERPRISE:
                        if (ctap21_enable_enterprise()) {
                            resp.status = 0x00;
                        } else {
                            resp.status = CTAP21_ERR_NOT_ALLOWED;
                        }
                        break;
                        
                    case CONFIG_CMD_TOGGLE_ALWAYS_UV:
                        if (ctap21_toggle_always_uv(req->param1)) {
                            resp.status = 0x00;
                        } else {
                            resp.status = CTAP21_ERR_INVALID_PARAMETER;
                        }
                        break;
                        
                    case CONFIG_CMD_SET_MIN_PIN_LENGTH:
                        if (ctap21_set_min_pin_length(req->param1)) {
                            resp.status = 0x00;
                        } else {
                            resp.status = CTAP21_ERR_PIN_POLICY_VIOLATION;
                        }
                        break;
                        
                    default:
                        resp.status = CTAP21_ERR_INVALID_PARAMETER;
                        break;
                }
                
                response[0] = CTAP21_CONFIG;
                response[1] = resp.status;
                response_len = 2;
            }
            break;
            
        default:
            response[0] = command;
            response[1] = CTAP21_ERR_INVALID_COMMAND;
            response_len = 2;
            break;
    }
    
    fido2_send_response(response, response_len);
}

// Credential Management

bool ctap21_get_credentials(uint8_t *rp_id, uint8_t rp_len, credential_mgmt_resp_t *resp) {
    printf("CTAP2.1: Getting credentials for RP: %.*s\n", rp_len, rp_id);
    
    uint8_t count = 0;
    
    // Filter credentials by RP ID
    for (int i = 0; i < ctap21_state.credential_count && count < 10; i++) {
        if (memcmp(ctap21_state.resident_keys[i].rp_id, rp_id, rp_len) == 0) {
            memcpy(&resp->credentials[count], &ctap21_state.resident_keys[i], sizeof(resident_key_t));
            count++;
        }
    }
    
    resp->credential_count = count;
    resp->status = count > 0 ? 0x00 : CTAP21_ERR_NO_CREDENTIALS;
    
    return count > 0;
}

bool ctap21_delete_credential(uint8_t *credential_id) {
    printf("CTAP2.1: Deleting credential\n");
    
    for (int i = 0; i < ctap21_state.credential_count; i++) {
        if (memcmp(ctap21_state.resident_keys[i].credential_id, credential_id, 32) == 0) {
            // Remove by shifting
            for (int j = i; j < ctap21_state.credential_count - 1; j++) {
                memcpy(&ctap21_state.resident_keys[j], &ctap21_state.resident_keys[j + 1], sizeof(resident_key_t));
            }
            ctap21_state.credential_count--;
            memset(&ctap21_state.resident_keys[ctap21_state.credential_count], 0, sizeof(resident_key_t));
            return true;
        }
    }
    return false;
}

bool ctap21_update_user(uint8_t *credential_id, uint8_t *user_name, uint8_t name_len) {
    printf("CTAP2.1: Updating user for credential\n");
    
    for (int i = 0; i < ctap21_state.credential_count; i++) {
        if (memcmp(ctap21_state.resident_keys[i].credential_id, credential_id, 32) == 0) {
            memset(ctap21_state.resident_keys[i].user_name, 0, 64);
            memcpy(ctap21_state.resident_keys[i].user_name, user_name, name_len > 64 ? 64 : name_len);
            return true;
        }
    }
    return false;
}

bool ctap21_get_credentials_metadata(uint8_t *rp_id, uint8_t rp_len, uint8_t *count) {
    uint8_t c = 0;
    for (int i = 0; i < ctap21_state.credential_count; i++) {
        if (memcmp(ctap21_state.resident_keys[i].rp_id, rp_id, rp_len) == 0) {
            c++;
        }
    }
    *count = c;
    return true;
}

// Configuration

bool ctap21_enable_enterprise(void) {
    printf("CTAP2.1: Enabling enterprise attestation\n");
    
    if (!ctap21_state.options.enterprise) {
        ctap21_state.options.enterprise = true;
        ctap21_state.enterprise_enabled = true;
        return true;
    }
    
    return false;
}

bool ctap21_toggle_always_uv(bool enable) {
    printf("CTAP2.1: Toggling always UV: %s\n", enable ? "ON" : "OFF");
    
    ctap21_state.always_uv = enable;
    ctap21_state.options.uv = enable;
    
    return true;
}

bool ctap21_set_min_pin_length(uint8_t length) {
    printf("CTAP2.1: Setting min PIN length: %u\n", length);
    
    if (length < 4 || length > 64) {
        return false;
    }
    
    ctap21_state.min_pin_length = length;
    return true;
}

bool ctap21_vendor_prototype(uint8_t *data, uint8_t len) {
    printf("CTAP2.1: Vendor prototype command\n");
    // Implementation specific
    return true;
}

// Selection

bool ctap21_selection_get(uint8_t timeout) {
    printf("CTAP2.1: Selection get (timeout: %u)\n", timeout);
    
    // Simulate user selection
    sleep_ms(timeout * 100);
    
    return true;
}

bool ctap21_selection_select(uint8_t index) {
    printf("CTAP2.1: Selection select index: %u\n", index);
    
    if (index >= ctap21_state.credential_count) {
        return false;
    }
    
    return true;
}

// Get Next Assertion

bool ctap21_get_next_assertion(uint8_t index, uint8_t *assertion, uint8_t *assertion_len) {
    printf("CTAP2.1: Get next assertion index: %u\n", index);
    
    if (index >= ctap21_state.credential_count) {
        return false;
    }
    
    // Build assertion (simplified)
    *assertion_len = 64;
    for (int i = 0; i < 64; i++) {
        assertion[i] = (uint8_t)(index + i);
    }
    
    return true;
}

// Bio Info

bool ctap21_bio_get_info(ctap21_bio_info_t *info) {
    printf("CTAP2.1: Getting bio info\n");
    
    info->modality = MODALITY_FINGERPRINT;
    info->fingerprint_type = SENSOR_TYPE_TOUCH;
    info->max_fingerprints = MAX_FINGERPRINTS;
    info->current_fingerprints = get_fingerprint_count();
    info->capabilities = CAPABILITY_TIMEOUT | CAPABILITY_USER_VER | CAPABILITY_ENROLL;
    
    return true;
}

// Extended Options

bool ctap21_get_options(ctap21_options_t *options) {
    memcpy(options, &ctap21_state.options, sizeof(ctap21_options_t));
    return true;
}

bool ctap21_set_options(ctap21_options_t *options) {
    memcpy(&ctap21_state.options, options, sizeof(ctap21_options_t));
    return true;
}

// Pin Policy

bool ctap21_validate_pin_policy(uint8_t *pin, uint8_t pin_len) {
    printf("CTAP2.1: Validating PIN policy\n");
    
    if (pin_len < ctap21_state.min_pin_length) {
        return false;
    }
    
    // Check for common PINs
    if (pin_len == 4) {
        if (memcmp(pin, "0000", 4) == 0 || memcmp(pin, "1234", 4) == 0) {
            return false;
        }
    }
    
    return true;
}

// Enterprise Attestation

bool ctap21_enterprise_attest(uint8_t *client_data, uint8_t data_len, uint8_t *attestation, uint8_t *att_len) {
    printf("CTAP2.1: Enterprise attestation\n");
    
    if (!ctap21_state.enterprise_enabled) {
        return false;
    }
    
    // Generate enterprise attestation (simplified)
    *att_len = 128;
    for (int i = 0; i < 128; i++) {
        attestation[i] = (uint8_t)(client_data[i % data_len] + i);
    }
    
    return true;
}

// Resident Key Operations

bool ctap21_store_resident_key(resident_key_t *rk) {
    if (ctap21_state.credential_count >= 10) {
        return false;
    }
    
    memcpy(&ctap21_state.resident_keys[ctap21_state.credential_count], rk, sizeof(resident_key_t));
    ctap21_state.credential_count++;
    
    printf("CTAP2.1: Stored resident key %u\n", ctap21_state.credential_count);
    return true;
}

bool ctap21_retrieve_resident_key(uint8_t *credential_id, resident_key_t *rk) {
    for (int i = 0; i < ctap21_state.credential_count; i++) {
        if (memcmp(ctap21_state.resident_keys[i].credential_id, credential_id, 32) == 0) {
            memcpy(rk, &ctap21_state.resident_keys[i], sizeof(resident_key_t));
            return true;
        }
    }
    return false;
}

bool ctap21_list_resident_keys(credential_mgmt_resp_t *resp) {
    resp->credential_count = ctap21_state.credential_count;
    memcpy(resp->credentials, ctap21_state.resident_keys, sizeof(resident_key_t) * ctap21_state.credential_count);
    resp->status = 0x00;
    return true;
}

// CTAP2.1 Specific Helpers

uint8_t ctap21_map_to_ctap2_error(uint8_t internal_error) {
    // Map internal errors to CTAP2.1 errors
    return internal_error;
}

bool ctap21_validate_request_size(uint16_t size, uint16_t max_size) {
    if (size > max_size) {
        return false;
    }
    return true;
}

bool ctap21_check_always_uv(void) {
    return ctap21_state.always_uv;
}

bool ctap21_check_pin_token_expired(void) {
    if (ctap21_state.pin_token_expiry == 0) {
        return true;
    }
    
    uint64_t now = time_us_64();
    return now > ctap21_state.pin_token_expiry;
}