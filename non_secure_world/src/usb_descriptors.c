#include "tusb.h"

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t const desc_device = {.bLength = sizeof(tusb_desc_device_t),
                                        .bDescriptorType = TUSB_DESC_DEVICE,
                                        .bcdUSB = 0x0200,
                                        .bDeviceClass = 0x00,
                                        .bDeviceSubClass = 0x00,
                                        .bDeviceProtocol = 0x00,
                                        .bMaxPacketSize0 =
                                            CFG_TUD_ENDPOINT0_SIZE,

                                        .idVendor = 0xCafe,
                                        .idProduct = 0x4000,
                                        .bcdDevice = 0x0100,

                                        .iManufacturer = 0x01,
                                        .iProduct = 0x02,
                                        .iSerialNumber = 0x03,

                                        .bNumConfigurations = 0x01};

uint8_t const *tud_descriptor_device_cb(void) {
  return (uint8_t const *)&desc_device;
}

//--------------------------------------------------------------------+
// CCID Descriptor Definitions
//--------------------------------------------------------------------+

#define TUD_CCID_DESC_LEN (9 + 54 + 7 + 7)

// CCID Class Descriptor (Smart Card Device Class Descriptor)
// Length: 54 bytes, Type: 0x21, Ver: 1.10
// See CCID Rev 1.1 Table 5.1-1
#define TUD_CCID_DESCRIPTOR(_itfnum, _stridx, _epout, _epin, _epsize)          \
  /* Interface Descriptor */                                                   \
  9, TUSB_DESC_INTERFACE, _itfnum, 0, 2, 0x0B /* Smart Card */, 0x00, 0x00,    \
      _stridx,                /* CCID Class Descriptor */                      \
      54, 0x21, 0x10, 0x01,   /* bcdCCID 1.10 */                               \
      0x00,                   /* bMaxSlotIndex */                              \
      0x07,                   /* bVoltageSupport (5.0V, 3.0V, 1.8V) */         \
      0x03, 0x00, 0x00, 0x00, /* dwProtocols (T=0, T=1) */                     \
      0xA0, 0x0F, 0x00, 0x00, /* dwDefaultClock (4000 kHz) */                  \
      0xA0, 0x0F, 0x00, 0x00, /* dwMaximumClock (4000 kHz) */                  \
      0x00,                   /* bNumClockSupported */                         \
      0x80, 0x25, 0x00, 0x00, /* dwDataRate (9600 bps) */                      \
      0x80, 0x25, 0x00, 0x00, /* dwMaxDataRate (9600 bps) */                   \
      0x00,                   /* bNumDataRatesSupported */                     \
      0xFE, 0x00, 0x00, 0x00, /* dwMaxIFSD (254) */                            \
      0x00, 0x00, 0x00, 0x00, /* dwSynchProtocols */                           \
      0x00, 0x00, 0x00, 0x00, /* dwMechanical */                               \
      0xBA, 0x04, 0x02,                                                        \
      0x00, /* dwFeatures (Auto config, TPDU, extended APDU) */                \
      0x00, 0x01, 0x00, 0x00,       /* dwMaxCCIDMessageLength (261) */         \
      0xFF,                         /* bClassGetResponse (Echo) */             \
      0xFF,                         /* bClassEnvelope (Echo) */                \
      0x00, 0x00,                   /* wLcdLayout */                           \
      0x00,                         /* bPINSupport (None) */                   \
      0x01, /* bMaxCCIDBusySlots */ /* Endpoint Out */                         \
      7, TUSB_DESC_ENDPOINT, _epout, TUSB_XFER_BULK, U16_TO_U8S_LE(_epsize),   \
      0, /* Endpoint In */                                                     \
      7, TUSB_DESC_ENDPOINT, _epin, TUSB_XFER_BULK, U16_TO_U8S_LE(_epsize), 0

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

enum { ITF_NUM_CCID, ITF_NUM_TOTAL };

#define EPNUM_CCID_OUT 0x02
#define EPNUM_CCID_IN 0x82

uint8_t const desc_configuration[] = {
    // Config number, interface count, string index, total length, attribute,
    // power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0,
                          TUD_CONFIG_DESC_LEN + TUD_CCID_DESC_LEN,
                          TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),

    // Interface number, string index, EP Out & EP In address, EP size
    TUD_CCID_DESCRIPTOR(ITF_NUM_CCID, 4, EPNUM_CCID_OUT, EPNUM_CCID_IN, 64),
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
  (void)index; // for multiple configurations
  return desc_configuration;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// array of pointer to string descriptors
char const *string_desc_arr[] = {
    (const char[]){0x09, 0x04}, // 0: is supported language is English (0x0409)
    "Raspberry Pi",             // 1: Manufacturer
    "RP2350 OATH",              // 2: Product
    "123456",                   // 3: Serials, should use chip ID
    "CCID Interface",           // 4: Interface
};

static uint16_t _desc_str[32];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
  (void)langid;
  uint8_t chr_count;

  if (index == 0) {
    memcpy(&_desc_str[1], string_desc_arr[0], 2);
    chr_count = 1;
  } else {
    if (!(index < sizeof(string_desc_arr) / sizeof(string_desc_arr[0])))
      return NULL;

    const char *str = string_desc_arr[index];

    // Cap at max char
    chr_count = (uint8_t)strlen(str);
    if (chr_count > 31)
      chr_count = 31;

    for (uint8_t i = 0; i < chr_count; i++) {
      _desc_str[1 + i] = str[i];
    }
  }

  // first byte is length (including header), second byte is string type
  _desc_str[0] = (TUSB_DESC_STRING << 8) | (2 * chr_count + 2);

  return _desc_str;
}
