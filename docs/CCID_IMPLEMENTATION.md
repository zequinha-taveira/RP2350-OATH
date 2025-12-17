# CCID Implementation Report: RP2350-OATH

This document describes how the RP2350-OATH firmware emulates a USB CCID (Chip Card Interface Device) to provide Smart Card functionality.

## 1. Device Identification

To be recognized by Yubico software and standard Smart Card drivers, the device uses specific descriptors:

- **Vendor ID (VID)**: `0x1050` (Yubico)
- **Product ID (PID)**: `0x0407` (YubiKey 5 Series CCID+FIDO+OTP)
- **ATR (Answer To Reset)**: `3B F8 13 00 00 81 31 FE 15 59 75 62 69 4B 65 79 34 D4`
  - This specific ATR informs the OS that the "card" is a YubiKey 5 with specific capabilities.

## 2. CCID Protocol Support

The firmware implements the following core CCID message types (PC to Reader):

| Message Type | Hex | Description |
|--------------|-----|-------------|
| `PC_TO_RDR_ICCPOWERON` | `0x62` | Powers on the emulated card, returns ATR. |
| `PC_TO_RDR_ICCPOWEROFF` | `0x63` | Powers off/resets the card state. |
| `PC_TO_RDR_XFRBLOCK` | `0x6F` | Transmits an APDU (Smart Card Command). |
| `PC_TO_RDR_GETSLOTSTATUS` | `0x65` | Checks if the card is present (Always returns PRESENT). |
| `PC_TO_RDR_GETPARAMETERS` | `0x6C` | Returns protocol parameters (T=1). |
| `PC_TO_RDR_SETPARAMETERS` | `0x61` | Configures protocol parameters. |
| `PC_TO_RDR_RESETPARAMETERS` | `0x6D` | Resets protocol parameters to defaults. |

## 3. APDU Integration

CCID `XFRBLOCK` messages contain APDU commands which are routed to the **Secure World** via the **Secure Gateway**.

- **OATH Application**: Handled by `oath_protocol.c`.
- **Status Words**: Standard ISO 7816-4 status words are used:
  - `90 00`: Success
  - `6A 82`: File/Application Not Found
  - `69 82`: Security Status Not Satisfied (PIN Required)

## 4. Driver Compatibility

By implementing the `GET/SET PARAMETERS` stubs and providing a valid YubiKey ATR, the device is natively compatible with:
- **Windows Smart Card Service (Base CSP)**
- **macOS Smart Card Services (CryptoTokenKit)**
- **Linux pcsc-lite (libccid)**
- **Yubico Authenticator**
- **YubiKey Manager (`ykman`)**
