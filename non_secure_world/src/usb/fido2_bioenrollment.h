#ifndef _FIDO2_BIOENROLLMENT_H_
#define _FIDO2_BIOENROLLMENT_H_

#include <stdint.h>
#include <stdbool.h>

// BioEnrollment Command (CTAP2.1)
#define CTAP2_BIO_ENROLL 0x09

// BioEnrollment Subcommands
#define BIO_ENROLL_ENROLL          0x01
#define BIO_ENROLL_ENUMERATE       0x02
#define BIO_ENROLL_REMOVE          0x03
#define BIO_ENROLL_SET_NAME        0x04
#define BIO_ENROLL_GET_INFO        0x05

// BioEnrollment Status Codes
#define BIO_STATUS_SUCCESS              0x00
#define BIO_STATUS_IN_PROGRESS          0x01
#define BIO_STATUS_CANCELED             0x02
#define BIO_STATUS_TIMEOUT              0x03
#define BIO_STATUS_DEVICE_LOCKED        0x04
#define BIO_STATUS_NOT_SUPPORTED        0x05
#define BIO_STATUS_CREDENTIAL_EXISTS    0x06
#define BIO_STATUS_CREDENTIAL_NOT_FOUND 0x07
#define BIO_STATUS_MAX_CREDENTIALS      0x08
#define BIO_STATUS_DATABASE_FULL        0x09

// Fingerprint Modality
#define MODALITY_FINGERPRINT     0x01
#define MODALITY_FACE            0x02
#define MODALITY_VOICE           0x03
#define MODALITY_IRIS            0x04

// Fingerprint Sensor Types
#define SENSOR_TYPE_TOUCH        0x01
#define SENSOR_TYPE_SWIPE        0x02
#define SENSOR_TYPE_PRESS        0x03

// Fingerprint Capabilities
#define CAPABILITY_TIMEOUT       0x01
#define CAPABILITY_USER_VER      0x02
#define CAPABILITY_ENROLL        0x04

// Maximum values
#define MAX_BIO_CREDENTIALS      5
#define MAX_FINGERPRINTS         5
#define MAX_BIO_NAME_LEN         64

// BioEnrollment Result
typedef struct __attribute__((packed)) {
    uint8_t status;
    uint8_t template_id_len;
    uint8_t template_id[32];
    uint8_t last_enroll_status;
    uint8_t remaining_samples;
} bio_enroll_result_t;

// Bio Credential Info
typedef struct __attribute__((packed)) {
    uint8_t template_id[32];
    uint8_t name_len;
    uint8_t name[MAX_BIO_NAME_LEN];
    uint8_t modality;
    uint8_t timestamp[8];
} bio_credential_info_t;

// BioEnrollment Request
typedef struct __attribute__((packed)) {
    uint8_t command;
    uint8_t subcommand;
    uint8_t modality;
    uint8_t name_len;
    uint8_t name[MAX_BIO_NAME_LEN];
    uint8_t template_id_len;
    uint8_t template_id[32];
} bio_enroll_req_t;

// BioEnrollment Response
typedef struct __attribute__((packed)) {
    uint8_t command;
    uint8_t status;
    uint8_t data_len;
    uint8_t data[256];
} bio_enroll_resp_t;

// Fingerprint Template
typedef struct __attribute__((packed)) {
    uint8_t template_id[32];
    uint8_t template_data[128];
    uint8_t name[MAX_BIO_NAME_LEN];
    uint8_t quality_score;
    uint8_t attempts;
    uint8_t timestamp[8];
} fingerprint_template_t;

// Function prototypes
void fido2_bioenrollment_init(void);
void fido2_bioenrollment_handle_command(uint8_t const *data, uint16_t len);

// BioEnrollment Operations
bool bio_enroll_start(uint8_t modality, uint8_t *name, uint8_t name_len, bio_enroll_result_t *result);
bool bio_enroll_process_sample(uint8_t *sample, uint8_t sample_len, bio_enroll_result_t *result);
bool bio_enroll_cancel(void);
bool bio_enroll_set_name(uint8_t *template_id, uint8_t *name, uint8_t name_len);
bool bio_enroll_remove(uint8_t *template_id);
bool bio_enumerate_credentials(bio_credential_info_t *credentials, uint8_t *count);

// Fingerprint-specific functions
bool fingerprint_enroll_start(uint8_t *name, uint8_t name_len);
bool fingerprint_enroll_add_sample(uint8_t *sample, uint8_t sample_len);
bool fingerprint_enroll_complete(void);
uint8_t fingerprint_get_quality(void);

// Template Management
bool store_fingerprint_template(fingerprint_template_t *templ);
bool retrieve_fingerprint_template(uint8_t *template_id, fingerprint_template_t *templ);
bool delete_fingerprint_template(uint8_t *template_id);
uint8_t get_fingerprint_count(void);

// User Verification
bool bio_verify_user(void);
bool bio_verify_fingerprint(void);
bool bio_verify_face(void);

// Hardware Interface
bool bio_sensor_init(void);
bool bio_sensor_capture(uint8_t *sample, uint8_t *sample_len);
bool bio_sensor_calibrate(void);
uint8_t bio_sensor_get_quality(void);

#endif // _FIDO2_BIOENROLLMENT_H_