#ifndef CBOR_H
#define CBOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


typedef enum {
  CBOR_TYPE_UINT = 0x00,
  CBOR_TYPE_NINT = 0x20,
  CBOR_TYPE_BYTES = 0x40,
  CBOR_TYPE_TEXT = 0x60,
  CBOR_TYPE_ARRAY = 0x80,
  CBOR_TYPE_MAP = 0xA0,
  CBOR_TYPE_TAG = 0xC0,
  CBOR_TYPE_FLOAT_SIMPLE = 0xE0
} cbor_type_t;

uint8_t *cbor_encode_uint(uint8_t *buffer, uint64_t value);
uint8_t *cbor_encode_nint(uint8_t *buffer, uint64_t value);
uint8_t *cbor_encode_bytes(uint8_t *buffer, const uint8_t *data, size_t len);
uint8_t *cbor_encode_text(uint8_t *buffer, const char *text);
uint8_t *cbor_encode_array(uint8_t *buffer, size_t size);
uint8_t *cbor_encode_map(uint8_t *buffer, size_t size);
uint8_t *cbor_encode_bool(uint8_t *buffer, bool value);

// Simple parser state
typedef struct {
  const uint8_t *buffer;
  size_t size;
  size_t offset;
} cbor_parser_t;

bool cbor_parse_uint(cbor_parser_t *parser, uint64_t *value);
bool cbor_parse_map(cbor_parser_t *parser, size_t *size);
bool cbor_parse_array(cbor_parser_t *parser, size_t *size);
bool cbor_parse_bytes(cbor_parser_t *parser, const uint8_t **data, size_t *len);
bool cbor_parse_text(cbor_parser_t *parser, const char **text, size_t *len);

#endif // CBOR_H
