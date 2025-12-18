#include "cbor.h"
#include <string.h>

static uint8_t *encode_header(uint8_t *buffer, uint8_t type, uint64_t value) {
  if (value < 24) {
    *buffer++ = type | (uint8_t)value;
  } else if (value < 256) {
    *buffer++ = type | 24;
    *buffer++ = (uint8_t)value;
  } else if (value < 65536) {
    *buffer++ = type | 25;
    *buffer++ = (uint8_t)(value >> 8);
    *buffer++ = (uint8_t)value;
  } else if (value < 4294967296ULL) {
    *buffer++ = type | 26;
    *buffer++ = (uint8_t)(value >> 24);
    *buffer++ = (uint8_t)(value >> 16);
    *buffer++ = (uint8_t)(value >> 8);
    *buffer++ = (uint8_t)value;
  } else {
    *buffer++ = type | 27;
    *buffer++ = (uint8_t)(value >> 56);
    *buffer++ = (uint8_t)(value >> 48);
    *buffer++ = (uint8_t)(value >> 40);
    *buffer++ = (uint8_t)(value >> 32);
    *buffer++ = (uint8_t)(value >> 24);
    *buffer++ = (uint8_t)(value >> 16);
    *buffer++ = (uint8_t)(value >> 8);
    *buffer++ = (uint8_t)value;
  }
  return buffer;
}

uint8_t *cbor_encode_uint(uint8_t *buffer, uint64_t value) {
  return encode_header(buffer, CBOR_TYPE_UINT, value);
}

uint8_t *cbor_encode_nint(uint8_t *buffer, uint64_t value) {
  return encode_header(buffer, CBOR_TYPE_NINT, value);
}

uint8_t *cbor_encode_bytes(uint8_t *buffer, const uint8_t *data, size_t len) {
  buffer = encode_header(buffer, CBOR_TYPE_BYTES, len);
  memcpy(buffer, data, len);
  return buffer + len;
}

uint8_t *cbor_encode_text(uint8_t *buffer, const char *text) {
  size_t len = strlen(text);
  buffer = encode_header(buffer, CBOR_TYPE_TEXT, len);
  memcpy(buffer, text, len);
  return buffer + len;
}

uint8_t *cbor_encode_array(uint8_t *buffer, size_t size) {
  return encode_header(buffer, CBOR_TYPE_ARRAY, size);
}

uint8_t *cbor_encode_map(uint8_t *buffer, size_t size) {
  return encode_header(buffer, CBOR_TYPE_MAP, size);
}

uint8_t *cbor_encode_bool(uint8_t *buffer, bool value) {
  *buffer++ = CBOR_TYPE_FLOAT_SIMPLE | (value ? 21 : 20);
  return buffer;
}

static bool parse_header(cbor_parser_t *parser, uint8_t expected_type,
                         uint64_t *value) {
  if (parser->offset >= parser->size)
    return false;

  uint8_t header = parser->buffer[parser->offset++];
  if ((header & 0xE0) != expected_type) {
    parser->offset--; // Backtrack
    return false;
  }

  uint8_t info = header & 0x1F;
  if (info < 24) {
    *value = info;
  } else if (info == 24) {
    if (parser->offset + 1 > parser->size)
      return false;
    *value = parser->buffer[parser->offset++];
  } else if (info == 25) {
    if (parser->offset + 2 > parser->size)
      return false;
    *value = (uint64_t)parser->buffer[parser->offset++] << 8;
    *value |= parser->buffer[parser->offset++];
  } else if (info == 26) {
    if (parser->offset + 4 > parser->size)
      return false;
    *value = (uint64_t)parser->buffer[parser->offset++] << 24;
    *value |= (uint64_t)parser->buffer[parser->offset++] << 16;
    *value |= (uint64_t)parser->buffer[parser->offset++] << 8;
    *value |= parser->buffer[parser->offset++];
  } else if (info == 27) {
    if (parser->offset + 8 > parser->size)
      return false;
    *value = (uint64_t)parser->buffer[parser->offset++] << 56;
    *value |= (uint64_t)parser->buffer[parser->offset++] << 48;
    *value |= (uint64_t)parser->buffer[parser->offset++] << 40;
    *value |= (uint64_t)parser->buffer[parser->offset++] << 32;
    *value |= (uint64_t)parser->buffer[parser->offset++] << 24;
    *value |= (uint64_t)parser->buffer[parser->offset++] << 16;
    *value |= (uint64_t)parser->buffer[parser->offset++] << 8;
    *value |= parser->buffer[parser->offset++];
  } else {
    return false; // Unsupported info
  }
  return true;
}

bool cbor_parse_uint(cbor_parser_t *parser, uint64_t *value) {
  return parse_header(parser, CBOR_TYPE_UINT, value);
}

bool cbor_parse_map(cbor_parser_t *parser, size_t *size) {
  uint64_t val;
  if (parse_header(parser, CBOR_TYPE_MAP, &val)) {
    *size = (size_t)val;
    return true;
  }
  return false;
}

bool cbor_parse_array(cbor_parser_t *parser, size_t *size) {
  uint64_t val;
  if (parse_header(parser, CBOR_TYPE_ARRAY, &val)) {
    *size = (size_t)val;
    return true;
  }
  return false;
}

bool cbor_parse_bytes(cbor_parser_t *parser, const uint8_t **data,
                      size_t *len) {
  uint64_t val;
  if (parse_header(parser, CBOR_TYPE_BYTES, &val)) {
    *len = (size_t)val;
    if (parser->offset + *len > parser->size)
      return false;
    *data = &parser->buffer[parser->offset];
    parser->offset += *len;
    return true;
  }
  return false;
}

bool cbor_parse_text(cbor_parser_t *parser, const char **text, size_t *len) {
  uint64_t val;
  if (parse_header(parser, CBOR_TYPE_TEXT, &val)) {
    *len = (size_t)val;
    if (parser->offset + *len > parser->size)
      return false;
    *text = (const char *)&parser->buffer[parser->offset];
    parser->offset += *len;
    return true;
  }
  return false;
}
