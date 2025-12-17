# Reverse Engineering Report: Yubico OATH Compatibility

This document outlines the technical findings from reverse engineering the Yubico OATH protocol (YKOATH) and its interaction with Yubico software tools. These findings were used to create the **RP2350-YKOATH** fork.

> [!NOTE]
> **A fork is a Yubico-compatible copy of the original repository.** This refers to the main open-source software repositories provided by Yubico themselves, as their products are designed to be used with their own and compatible open-source tools. This project specifically focuses on total Yubico compatibility by aligning with the official open-source protocols and tools maintained in **Yubico's GitHub repositories**.

## 1. USB Interface Requirements

### VID/PID Identification
Yubico tools (Authenticator, `ykman`) look for specific Vendor and Product IDs to identify a YubiKey.
- **Vendor ID (VID)**: `0x1050` (Yubico)
- **Product ID (PID)**: `0x0407` (YubiKey 5 Series CCID+FIDO+OTP)

### USB Descriptors
- **Manufacturer String**: "Yubico"
- **Product String**: "YubiKey 5 FIDO+CCID"
- **Serial Number**: A unique string. Official YubiKeys use a decimal or hex string derived from the internal serial.

## 2. OATH Protocol Details (CCID/APDU)

### Application Selection
When selecting the OATH application (`AID: A0 00 00 05 27 20 01`), the software expects a version response.
- **Expected Tag**: `0x79` (Version)
- **Format**: `79 03 [Major] [Minor] [Patch]`
- **Status Word**: `90 00` (Success)

### Missing Instructions
The following instructions are critical for tool compatibility:
- **`INS_VERSION` (`0x06`)**: Returns the device version (e.g., `05 04 03`).
- **Management AID**: Many tools attempt to select the Management app (`A0 00 00 05 27 47 11 17`) to check for device capabilities. Returning `90 00` even for an empty implementation improves compatibility.

### APDU Command Mapping
Official YKOATH uses the following instruction mapping:
- `0x01`: PUT (Add credential)
- `0x02`: DELETE (Remove credential)
- `0x03`: SET CODE (Set PIN)
- `0x04`: RESET (Wipe device)
- `0xA1`: CALCULATE / LIST (Differentiated by tags in Data field)
- `0xA3`: VALIDATE (Verify PIN)

## 3. Implementation Summary for RP2350

1. **Bug Fix**: The original `CALCULATE` handler had a misplaced `SW_NOT_IMPLEMENTED` return that blocked many legitimate requests.
2. **Standardization**: Responses for `LIST` and `CALCULATE` were adjusted to ensure strict TLV (Tag-Length-Value) alignment.
3. **Hardware Integration**: The RP2350's Unique Board ID is now used to generate a persistent and unique USB serial number.
