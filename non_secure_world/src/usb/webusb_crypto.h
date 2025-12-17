#ifndef _WEBUSB_CRYPTO_H_
#define _WEBUSB_CRYPTO_H_

#include <stdint.h>
#include <stdbool.h>

// WebUSB WebCrypto API Commands
#define WEBUSB_CRYPTO_CMD_GENERATE_KEY     0x10
#define WEBUSB_CRYPTO_CMD_IMPORT_KEY       0x11
#define WEBUSB_CRYPTO_CMD_EXPORT_KEY       0x12
#define WEBUSB_CRYPTO_CMD_SIGN             0x13
#define WEBUSB_CRYPTO_CMD_VERIFY           0x14
#define WEBUSB_CRYPTO_CMD_ENCRYPT          0x15
#define WEBUSB_CRYPTO_CMD_DECRYPT          0x16
#define WEBUSB_CRYPTO_CMD_DIGEST           0x17
#define WEBUSB_CRYPTO_CMD_DERIVE_KEY       0x18

// Key Types
#define KEY_TYPE_ECDSA_P256    0x01
#define KEY_TYPE_RSA_OAEP      0x02
#define KEY_TYPE_AES_GCM       0x03
#define KEY_TYPE_HMAC_SHA256   0x04

// Algorithm Identifiers
#define ALG_ECDSA_P256        0x01
#define ALG_RSA_OAEP_SHA256   0x02
#define ALG_AES_GCM_256       0x03
#define ALG_HMAC_SHA256       0x04
#define ALG_SHA256            0x05

// Response Status
#define CRYPTO_STATUS_SUCCESS      0x00
#define CRYPTO_STATUS_ERROR        0x01
#define CRYPTO_STATUS_INVALID_KEY  0x02
#define CRYPTO_STATUS_INVALID_OP   0x03
#define CRYPTO_STATUS_MEMORY       0x04

// Maximum sizes
#define MAX_KEY_SIZE      256
#define MAX_DATA_SIZE     512
#define MAX_SIGNATURE_SIZE 256

// Key structure
typedef struct __attribute__((packed)) {
    uint8_t key_type;
    uint8_t key_size;
    uint8_t key_data[MAX_KEY_SIZE];
} crypto_key_t;

// Generate Key Request
typedef struct __attribute__((packed)) {
    uint8_t command;
    uint8_t algorithm;
    uint8_t key_size;
} crypto_generate_req_t;

// Generate Key Response
typedef struct __attribute__((packed)) {
    uint8_t status;
    uint8_t key_id;
    uint8_t public_key[MAX_KEY_SIZE];
} crypto_generate_resp_t;

// Sign Request
typedef struct __attribute__((packed)) {
    uint8_t command;
    uint8_t key_id;
    uint8_t data_len;
    uint8_t data[MAX_DATA_SIZE];
} crypto_sign_req_t;

// Sign Response
typedef struct __attribute__((packed)) {
    uint8_t status;
    uint8_t signature_len;
    uint8_t signature[MAX_SIGNATURE_SIZE];
} crypto_sign_resp_t;

// Encrypt Request
typedef struct __attribute__((packed)) {
    uint8_t command;
    uint8_t key_id;
    uint8_t iv_len;
    uint8_t iv[16];
    uint8_t data_len;
    uint8_t data[MAX_DATA_SIZE];
} crypto_encrypt_req_t;

// Encrypt Response
typedef struct __attribute__((packed)) {
    uint8_t status;
    uint8_t data_len;
    uint8_t data[MAX_DATA_SIZE];
} crypto_encrypt_resp_t;

// Function prototypes
void webusb_crypto_init(void);
void webusb_crypto_handle_command(uint8_t const *msg, uint32_t len);

// Crypto operations
bool crypto_generate_key(uint8_t algorithm, uint8_t key_size, crypto_key_t *key);
bool crypto_sign(uint8_t key_id, uint8_t *data, uint8_t data_len, uint8_t *signature, uint8_t *sig_len);
bool crypto_verify(uint8_t key_id, uint8_t *data, uint8_t data_len, uint8_t *signature, uint8_t sig_len);
bool crypto_encrypt(uint8_t key_id, uint8_t *iv, uint8_t iv_len, uint8_t *data, uint8_t data_len, uint8_t *out, uint8_t *out_len);
bool crypto_decrypt(uint8_t key_id, uint8_t *iv, uint8_t iv_len, uint8_t *data, uint8_t data_len, uint8_t *out, uint8_t *out_len);
bool crypto_digest(uint8_t algorithm, uint8_t *data, uint8_t data_len, uint8_t *hash, uint8_t *hash_len);

// Key management
bool crypto_store_key(uint8_t key_id, crypto_key_t *key);
bool crypto_retrieve_key(uint8_t key_id, crypto_key_t *key);
bool crypto_delete_key(uint8_t key_id);

#endif // _WEBUSB_CRYPTO_H_