#include "webusb_crypto.h"
#include "pico/stdlib.h"
#include "secure_gateway.h"
#include "webusb_device.h"
#include <stdio.h>
#include <string.h>

// Simulated key storage (in production, use secure storage)
static crypto_key_t key_storage[16];
static uint8_t key_count = 0;

void webusb_crypto_init(void) {
  printf("WebUSB Crypto: Initialized\n");
  key_count = 0;
  memset(key_storage, 0, sizeof(key_storage));
}

void webusb_crypto_handle_command(uint8_t const *msg, uint32_t len) {
  if (len < 1)
    return;

  uint8_t command = msg[0];
  uint8_t response[512];
  uint16_t response_len = 0;

  printf("WebUSB Crypto: Command 0x%02X received\n", command);

  switch (command) {
  case WEBUSB_CRYPTO_CMD_GENERATE_KEY: {
    crypto_generate_req_t *req = (crypto_generate_req_t *)msg;
    crypto_key_t key;

    if (crypto_generate_key(req->algorithm, req->key_size, &key)) {
      uint8_t key_id = key_count++;
      crypto_store_key(key_id, &key);

      response[0] = WEBUSB_CRYPTO_CMD_GENERATE_KEY;
      response[1] = CRYPTO_STATUS_SUCCESS;
      response[2] = key_id;
      memcpy(response + 3, key.key_data, key.key_size);
      response_len = 3 + key.key_size;
    } else {
      response[0] = WEBUSB_CRYPTO_CMD_GENERATE_KEY;
      response[1] = CRYPTO_STATUS_ERROR;
      response_len = 2;
    }
    break;
  }

  case WEBUSB_CRYPTO_CMD_SIGN: {
    crypto_sign_req_t *req = (crypto_sign_req_t *)msg;
    uint8_t signature[256];
    uint8_t sig_len;

    if (crypto_sign(req->key_id, req->data, req->data_len, signature,
                    &sig_len)) {
      response[0] = WEBUSB_CRYPTO_CMD_SIGN;
      response[1] = CRYPTO_STATUS_SUCCESS;
      response[2] = sig_len;
      memcpy(response + 3, signature, sig_len);
      response_len = 3 + sig_len;
    } else {
      response[0] = WEBUSB_CRYPTO_CMD_SIGN;
      response[1] = CRYPTO_STATUS_ERROR;
      response_len = 2;
    }
    break;
  }

  case WEBUSB_CRYPTO_CMD_ENCRYPT: {
    crypto_encrypt_req_t *req = (crypto_encrypt_req_t *)msg;
    uint8_t out[512];
    uint8_t out_len;

    if (crypto_encrypt(req->key_id, req->iv, req->iv_len, req->data,
                       req->data_len, out, &out_len)) {
      response[0] = WEBUSB_CRYPTO_CMD_ENCRYPT;
      response[1] = CRYPTO_STATUS_SUCCESS;
      response[2] = out_len;
      memcpy(response + 3, out, out_len);
      response_len = 3 + out_len;
    } else {
      response[0] = WEBUSB_CRYPTO_CMD_ENCRYPT;
      response[1] = CRYPTO_STATUS_ERROR;
      response_len = 2;
    }
    break;
  }

  case WEBUSB_CRYPTO_CMD_DIGEST: {
    uint8_t algorithm = msg[1];
    uint8_t data_len = msg[2];
    uint8_t *data = (uint8_t *)(msg + 3);
    uint8_t hash[32];
    uint8_t hash_len;

    if (crypto_digest(algorithm, data, data_len, hash, &hash_len)) {
      response[0] = WEBUSB_CRYPTO_CMD_DIGEST;
      response[1] = CRYPTO_STATUS_SUCCESS;
      response[2] = hash_len;
      memcpy(response + 3, hash, hash_len);
      response_len = 3 + hash_len;
    } else {
      response[0] = WEBUSB_CRYPTO_CMD_DIGEST;
      response[1] = CRYPTO_STATUS_ERROR;
      response_len = 2;
    }
    break;
  }

  default:
    response[0] = command;
    response[1] = CRYPTO_STATUS_INVALID_OP;
    response_len = 2;
    break;
  }

  if (response_len > 0) {
    webusb_send_response(response, response_len);
  }
}

// Crypto Operations Implementation

bool crypto_generate_key(uint8_t algorithm, uint8_t key_size,
                         crypto_key_t *key) {
  printf("Crypto: Generating key via HSM - Alg: 0x%02X\n", algorithm);

  if (algorithm != ALG_ECDSA_P256) {
    printf("Crypto: Only ECDSA P-256 is supported via HSM currently\n");
    return false;
  }

  uint8_t status = 0;
  // For now, use key_count as slot index
  if (secure_gateway_hsm_gen_key(key_count, &status)) {
    key->key_type = algorithm;
    key->key_size = 64; // Public key size

    uint16_t pub_len = 0;
    if (secure_gateway_hsm_get_pubkey(key_count, key->key_data, &pub_len)) {
      key->key_size = (uint8_t)pub_len;
      return true;
    }
  }

  return false;
}

bool crypto_sign(uint8_t key_id, uint8_t *data, uint8_t data_len,
                 uint8_t *signature, uint8_t *sig_len) {
  if (key_id >= 16)
    return false;

  printf("Crypto: Signing data via HSM with slot %u\n", key_id);

  uint8_t hash[32];
  uint8_t hash_len_out;

  // WebCrypto Sign usually expects a hash or hashes it internally
  // For ECDSA P-256, we hash the data first
  if (!crypto_digest(ALG_SHA256, data, data_len, hash, &hash_len_out)) {
    return false;
  }

  uint16_t sig_len_out = 0;
  if (secure_gateway_hsm_sign(key_id, hash, signature, &sig_len_out)) {
    *sig_len = (uint8_t)sig_len_out;
    return true;
  }

  return false;
}

bool crypto_verify(uint8_t key_id, uint8_t *data, uint8_t data_len,
                   uint8_t *signature, uint8_t sig_len) {
  printf("Crypto: Verifying signature with key %u\n", key_id);
  // In real implementation, verify ECDSA signature
  return true;
}

bool crypto_encrypt(uint8_t key_id, uint8_t *iv, uint8_t iv_len, uint8_t *data,
                    uint8_t data_len, uint8_t *out, uint8_t *out_len) {
  crypto_key_t key;
  if (!crypto_retrieve_key(key_id, &key)) {
    return false;
  }

  printf("Crypto: Encrypting data with key %u\n", key_id);

  // Use AES-GCM from secure world
  // This would call secure_gateway_encrypt()
  *out_len = data_len;
  for (int i = 0; i < data_len; i++) {
    out[i] = data[i] ^ key.key_data[i % key.key_size] ^ iv[i % iv_len];
  }

  return true;
}

bool crypto_decrypt(uint8_t key_id, uint8_t *iv, uint8_t iv_len, uint8_t *data,
                    uint8_t data_len, uint8_t *out, uint8_t *out_len) {
  crypto_key_t key;
  if (!crypto_retrieve_key(key_id, &key)) {
    return false;
  }

  printf("Crypto: Decrypting data with key %u\n", key_id);

  *out_len = data_len;
  for (int i = 0; i < data_len; i++) {
    out[i] = data[i] ^ key.key_data[i % key.key_size] ^ iv[i % iv_len];
  }

  return true;
}

bool crypto_digest(uint8_t algorithm, uint8_t *data, uint8_t data_len,
                   uint8_t *hash, uint8_t *hash_len) {
  printf("Crypto: Computing digest via HSM placeholder - Alg: 0x%02X\n",
         algorithm);

  if (algorithm == ALG_SHA256) {
    *hash_len = 32;
    // Mock SHA-256 for now, in production use hardware accelerator
    for (int i = 0; i < 32; i++) {
      hash[i] = (uint8_t)(data[i % data_len] + i);
    }
    return true;
  }

  return false;
}

// Key Management

bool crypto_store_key(uint8_t key_id, crypto_key_t *key) {
  if (key_id >= 16)
    return false;

  memcpy(&key_storage[key_id], key, sizeof(crypto_key_t));
  printf("Crypto: Stored key %u (type: 0x%02X, size: %u)\n", key_id,
         key->key_type, key->key_size);
  return true;
}

bool crypto_retrieve_key(uint8_t key_id, crypto_key_t *key) {
  if (key_id >= 16)
    return false;
  if (key_storage[key_id].key_size == 0)
    return false;

  memcpy(key, &key_storage[key_id], sizeof(crypto_key_t));
  return true;
}

bool crypto_delete_key(uint8_t key_id) {
  if (key_id >= 16)
    return false;

  memset(&key_storage[key_id], 0, sizeof(crypto_key_t));
  printf("Crypto: Deleted key %u\n", key_id);
  return true;
}