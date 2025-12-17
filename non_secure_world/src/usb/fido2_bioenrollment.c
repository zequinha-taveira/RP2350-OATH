#include "fido2_bioenrollment.h"
#include "fido2_device.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


// BioEnrollment State
typedef struct {
  bool enroll_active;
  uint8_t enroll_step;
  uint8_t template_id[32];
  uint8_t template_name[MAX_BIO_NAME_LEN];
  uint8_t modality;
  uint8_t samples_collected;
  uint8_t samples_required;
  uint8_t quality_threshold;
  uint64_t timestamp;
} bio_state_t;

static bio_state_t bio_state = {.enroll_active = false,
                                .enroll_step = 0,
                                .samples_collected = 0,
                                .samples_required = 3,
                                .quality_threshold = 80};

// Fingerprint templates storage
static fingerprint_template_t fingerprint_templates[MAX_FINGERPRINTS];
static uint8_t fingerprint_count = 0;

void fido2_bioenrollment_init(void) {
  printf("FIDO2 BioEnrollment: Initialized\n");
  memset(&bio_state, 0, sizeof(bio_state));
  memset(fingerprint_templates, 0, sizeof(fingerprint_templates));
  fingerprint_count = 0;

  // Initialize simulated fingerprint sensor
  bio_sensor_init();
}

void fido2_bioenrollment_handle_command(uint8_t const *data, uint16_t len) {
  if (len < 2)
    return;

  uint8_t subcommand = data[0];
  uint8_t modality = data[1];

  printf("FIDO2 BioEnrollment: Subcommand 0x%02X, Modality 0x%02X\n",
         subcommand, modality);

  uint8_t response[256];
  uint16_t response_len = 0;

  switch (subcommand) {
  case BIO_ENROLL_ENROLL:
    if (modality == MODALITY_FINGERPRINT) {
      uint8_t name_len = data[2];
      uint8_t *name = (uint8_t *)(data + 3);

      bio_enroll_result_t result;
      if (bio_enroll_start(modality, name, name_len, &result)) {
        response[0] = CTAP2_BIO_ENROLL;
        response[1] = BIO_STATUS_IN_PROGRESS;
        response[2] = result.template_id_len;
        memcpy(response + 3, result.template_id, result.template_id_len);
        response[3 + result.template_id_len] = result.remaining_samples;
        response_len = 4 + result.template_id_len;
      } else {
        response[0] = CTAP2_BIO_ENROLL;
        response[1] = BIO_STATUS_NOT_SUPPORTED;
        response_len = 2;
      }
    }
    break;

  case BIO_ENROLL_ENUMERATE: {
    bio_credential_info_t credentials[5];
    uint8_t count;
    if (bio_enumerate_credentials(credentials, &count)) {
      response[0] = CTAP2_BIO_ENROLL;
      response[1] = BIO_STATUS_SUCCESS;
      response[2] = count;
      uint8_t *ptr = response + 3;
      for (int i = 0; i < count; i++) {
        memcpy(ptr, credentials[i].template_id, 32);
        ptr += 32;
        *ptr++ = credentials[i].name_len;
        memcpy(ptr, credentials[i].name, credentials[i].name_len);
        ptr += credentials[i].name_len;
        *ptr++ = credentials[i].modality;
      }
      response_len = ptr - response;
    } else {
      response[0] = CTAP2_BIO_ENROLL;
      response[1] = BIO_STATUS_NOT_SUPPORTED;
      response_len = 2;
    }
  } break;

  case BIO_ENROLL_REMOVE: {
    uint8_t template_id_len = data[2];
    uint8_t *template_id = (uint8_t *)(data + 3);

    if (bio_enroll_remove(template_id)) {
      response[0] = CTAP2_BIO_ENROLL;
      response[1] = BIO_STATUS_SUCCESS;
      response_len = 2;
    } else {
      response[0] = CTAP2_BIO_ENROLL;
      response[1] = BIO_STATUS_CREDENTIAL_NOT_FOUND;
      response_len = 2;
    }
  } break;

  case BIO_ENROLL_SET_NAME: {
    uint8_t template_id_len = data[2];
    uint8_t *template_id = (uint8_t *)(data + 3);
    uint8_t name_len = data[3 + template_id_len];
    uint8_t *name = (uint8_t *)(data + 4 + template_id_len);

    if (bio_enroll_set_name(template_id, name, name_len)) {
      response[0] = CTAP2_BIO_ENROLL;
      response[1] = BIO_STATUS_SUCCESS;
      response_len = 2;
    } else {
      response[0] = CTAP2_BIO_ENROLL;
      response[1] = BIO_STATUS_CREDENTIAL_NOT_FOUND;
      response_len = 2;
    }
  } break;

  default:
    response[0] = CTAP2_BIO_ENROLL;
    response[1] = BIO_STATUS_NOT_SUPPORTED;
    response_len = 2;
    break;
  }

  // Send response via FIDO2
  fido2_send_response(CTAPHID_MSG, response, response_len);
}

// BioEnrollment Operations

bool bio_enroll_start(uint8_t modality, uint8_t *name, uint8_t name_len,
                      bio_enroll_result_t *result) {
  if (bio_state.enroll_active) {
    return false;
  }

  printf("BioEnrollment: Starting enrollment for '%.*s'\n", name_len, name);

  bio_state.enroll_active = true;
  bio_state.enroll_step = 0;
  bio_state.modality = modality;
  bio_state.samples_collected = 0;
  bio_state.timestamp = time_us_64();

  // Generate template ID
  for (int i = 0; i < 32; i++) {
    bio_state.template_id[i] = (uint8_t)rand();
  }

  // Store name
  memset(bio_state.template_name, 0, MAX_BIO_NAME_LEN);
  memcpy(bio_state.template_name, name,
         name_len > MAX_BIO_NAME_LEN ? MAX_BIO_NAME_LEN : name_len);

  result->status = BIO_STATUS_IN_PROGRESS;
  result->template_id_len = 32;
  memcpy(result->template_id, bio_state.template_id, 32);
  result->last_enroll_status = 0x00;
  result->remaining_samples = bio_state.samples_required;

  printf("BioEnrollment: Template ID generated\n");
  return true;
}

bool bio_enroll_process_sample(uint8_t *sample, uint8_t sample_len,
                               bio_enroll_result_t *result) {
  if (!bio_state.enroll_active) {
    return false;
  }

  // Simulate fingerprint quality check
  uint8_t quality = bio_sensor_get_quality();

  if (quality < bio_state.quality_threshold) {
    result->status = BIO_STATUS_IN_PROGRESS;
    result->last_enroll_status = 0x01; // Low quality
    result->remaining_samples =
        bio_state.samples_required - bio_state.samples_collected;
    return true;
  }

  bio_state.samples_collected++;
  bio_state.enroll_step++;

  printf("BioEnrollment: Sample %u/%u collected (Quality: %u%%)\n",
         bio_state.samples_collected, bio_state.samples_required, quality);

  if (bio_state.samples_collected >= bio_state.samples_required) {
    // Enrollment complete
    fingerprint_template_t template;
    memcpy(template.template_id, bio_state.template_id, 32);
    memcpy(template.name, bio_state.template_name, MAX_BIO_NAME_LEN);
    template.quality_score = quality;
    template.attempts = bio_state.enroll_step;

    // Store timestamp
    uint64_t now = time_us_64();
    memcpy(template.timestamp, &now, 8);

    if (store_fingerprint_template(&template)) {
      result->status = BIO_STATUS_SUCCESS;
      result->last_enroll_status = 0x00;
      result->remaining_samples = 0;

      bio_state.enroll_active = false;
      printf("BioEnrollment: Completed successfully\n");
      return true;
    } else {
      result->status = BIO_STATUS_DATABASE_FULL;
      result->last_enroll_status = 0x02; // Storage error
      return false;
    }
  } else {
    result->status = BIO_STATUS_IN_PROGRESS;
    result->last_enroll_status = 0x00;
    result->remaining_samples =
        bio_state.samples_required - bio_state.samples_collected;
    return true;
  }
}

bool bio_enroll_cancel(void) {
  if (!bio_state.enroll_active) {
    return false;
  }

  printf("BioEnrollment: Canceled\n");
  bio_state.enroll_active = false;
  return true;
}

bool bio_enroll_set_name(uint8_t *template_id, uint8_t *name,
                         uint8_t name_len) {
  for (int i = 0; i < fingerprint_count; i++) {
    if (memcmp(fingerprint_templates[i].template_id, template_id, 32) == 0) {
      memset(fingerprint_templates[i].name, 0, MAX_BIO_NAME_LEN);
      memcpy(fingerprint_templates[i].name, name,
             name_len > MAX_BIO_NAME_LEN ? MAX_BIO_NAME_LEN : name_len);
      printf("BioEnrollment: Name updated for template\n");
      return true;
    }
  }
  return false;
}

bool bio_enroll_remove(uint8_t *template_id) {
  for (int i = 0; i < fingerprint_count; i++) {
    if (memcmp(fingerprint_templates[i].template_id, template_id, 32) == 0) {
      // Remove by shifting array
      for (int j = i; j < fingerprint_count - 1; j++) {
        memcpy(&fingerprint_templates[j], &fingerprint_templates[j + 1],
               sizeof(fingerprint_template_t));
      }
      fingerprint_count--;
      memset(&fingerprint_templates[fingerprint_count], 0,
             sizeof(fingerprint_template_t));
      printf("BioEnrollment: Template removed\n");
      return true;
    }
  }
  return false;
}

bool bio_enumerate_credentials(bio_credential_info_t *credentials,
                               uint8_t *count) {
  *count = fingerprint_count;

  for (int i = 0; i < fingerprint_count; i++) {
    memcpy(credentials[i].template_id, fingerprint_templates[i].template_id,
           32);
    credentials[i].name_len = MAX_BIO_NAME_LEN;
    memcpy(credentials[i].name, fingerprint_templates[i].name,
           MAX_BIO_NAME_LEN);
    credentials[i].modality = MODALITY_FINGERPRINT;
    memcpy(credentials[i].timestamp, fingerprint_templates[i].timestamp, 8);
  }

  return true;
}

// Fingerprint-specific functions

bool fingerprint_enroll_start(uint8_t *name, uint8_t name_len) {
  return bio_enroll_start(MODALITY_FINGERPRINT, name, name_len,
                          &(bio_enroll_result_t){0});
}

bool fingerprint_enroll_add_sample(uint8_t *sample, uint8_t sample_len) {
  bio_enroll_result_t result;
  return bio_enroll_process_sample(sample, sample_len, &result);
}

bool fingerprint_enroll_complete(void) {
  if (bio_state.samples_collected >= bio_state.samples_required) {
    bio_state.enroll_active = false;
    return true;
  }
  return false;
}

uint8_t fingerprint_get_quality(void) { return bio_sensor_get_quality(); }

// Template Management

bool store_fingerprint_template(fingerprint_template_t *templ) {
  if (fingerprint_count >= MAX_FINGERPRINTS) {
    return false;
  }

  memcpy(&fingerprint_templates[fingerprint_count], templ,
         sizeof(fingerprint_template_t));
  fingerprint_count++;

  printf("Fingerprint: Stored template %u\n", fingerprint_count);
  return true;
}

bool retrieve_fingerprint_template(uint8_t *template_id,
                                   fingerprint_template_t *templ) {
  for (int i = 0; i < fingerprint_count; i++) {
    if (memcmp(fingerprint_templates[i].template_id, template_id, 32) == 0) {
      memcpy(templ, &fingerprint_templates[i], sizeof(fingerprint_template_t));
      return true;
    }
  }
  return false;
}

bool delete_fingerprint_template(uint8_t *template_id) {
  return bio_enroll_remove(template_id);
}

uint8_t get_fingerprint_count(void) { return fingerprint_count; }

// User Verification

bool bio_verify_user(void) {
  printf("Bio: User verification requested\n");
  // In real implementation, wait for button press or biometric
  sleep_ms(500);
  return true;
}

bool bio_verify_fingerprint(void) {
  printf("Bio: Fingerprint verification\n");

  if (fingerprint_count == 0) {
    printf("Bio: No fingerprints enrolled\n");
    return false;
  }

  // Simulate fingerprint capture
  uint8_t sample[64];
  uint8_t sample_len;

  if (bio_sensor_capture(sample, &sample_len)) {
    // In real implementation, match against templates
    printf("Bio: Fingerprint captured, matching...\n");
    sleep_ms(300);
    return true;
  }

  return false;
}

bool bio_verify_face(void) {
  printf("Bio: Face verification\n");
  // Simulate face verification
  sleep_ms(500);
  return true;
}

// Hardware Interface

bool bio_sensor_init(void) {
  printf("Bio Sensor: Initialized\n");
  return true;
}

bool bio_sensor_capture(uint8_t *sample, uint8_t *sample_len) {
  // Simulate fingerprint capture
  *sample_len = 64;
  for (int i = 0; i < 64; i++) {
    sample[i] = (uint8_t)rand();
  }

  // Simulate sensor delay
  sleep_ms(200);

  printf("Bio Sensor: Sample captured\n");
  return true;
}

bool bio_sensor_calibrate(void) {
  printf("Bio Sensor: Calibrated\n");
  return true;
}

uint8_t bio_sensor_get_quality(void) {
  // Simulate quality measurement
  // Returns 0-100
  static uint8_t quality = 85;
  quality = (quality + (rand() % 10)) % 100;
  if (quality < 60)
    quality = 85; // Keep quality reasonable

  return quality;
}