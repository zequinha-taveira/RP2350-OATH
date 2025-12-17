import usb.core
import usb.util
import sys

# USB Device configuration
VENDOR_ID = 0x1209
PRODUCT_ID = 0x4D41

# WebUSB HSM Command IDs
CMD_HSM_GEN_KEY = 0x10
CMD_HSM_GET_PUBKEY = 0x11
CMD_HSM_SIGN = 0x12

def find_device():
    dev = usb.core.find(idVendor=VENDOR_ID, idProduct=PRODUCT_ID)
    if dev is None:
        print("Device not found")
        sys.exit(1)
    return dev

def test_hsm():
    dev = find_device()
    
    # Detach kernel driver if necessary
    if dev.is_kernel_driver_active(1):
        dev.detach_kernel_driver(1)
        
    dev.set_configuration()
    usb.util.claim_interface(dev, 1) # WebUSB Interface

    print("--- HSM GEN KEY (Slot 1) ---")
    # EP 3 OUT, EP 3 IN
    dev.write(0x03, [CMD_HSM_GEN_KEY, 0x01])
    resp = dev.read(0x83, 64)
    print(f"Status: {resp[1]:02X}")

    print("\n--- HSM GET PUBKEY (Slot 1) ---")
    dev.write(0x03, [CMD_HSM_GET_PUBKEY, 0x01])
    resp = dev.read(0x83, 64)
    if resp[1] == 0:
        pubkey = resp[2:]
        print(f"Public Key: {pubkey.hex()}")
    else:
        print(f"Error: {resp[1]:02X}")

    print("\n--- HSM SIGN (Slot 1) ---")
    hash_data = [0xAA] * 32
    dev.write(0x03, [CMD_HSM_SIGN, 0x01] + hash_data)
    resp = dev.read(0x83, 64)
    if resp[1] == 0:
        signature = resp[2:]
        print(f"Signature: {signature.hex()}")
    else:
        print(f"Error: {resp[1]:02X}")

    usb.util.release_interface(dev, 1)

if __name__ == "__main__":
    test_hsm()
