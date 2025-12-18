cmake_minimum_required(VERSION 3.13)

# Initialize SDK
set(PICO_TINYUSB_PATH ${CMAKE_CURRENT_LIST_DIR}/lib/tinyusb)
include(pico_sdk_import.cmake)

project(rp2350_oath_trustzone C CXX ASM)

# Set platform and board
set(PICO_PLATFORM rp2350 CACHE STRING "PICO PLATFORM")
set(PICO_BOARD tenstar_rp2350_usb CACHE STRING "PICO BOARD")
set(PICO_BOARD_HEADER_DIRS ${CMAKE_CURRENT_LIST_DIR} CACHE STRING "PICO BOARD HEADER DIRS")

# Initialize the SDK
pico_sdk_init()

# Add Secure World
add_subdirectory(secure_world)

# Use a specific variable to pass the import lib location to the Non-Secure World
# The secure_world CMakeLists.txt must produce this file.
set(SECURE_IMPORT_LIB ${CMAKE_CURRENT_BINARY_DIR}/secure_world/secure_app_import_lib.o)

# Add Non-Secure World
add_subdirectory(non_secure_world)

# --- SECURITY HARDENING: SECURE BOOT ---
# To enable Secure Boot on RP2350, you must:
# 1. Generate a public/private key pair using 'picotool'.
# 2. Burn the public key hash into the OTP.
# 3. Enable the following lines to sign the binary:

# set(PICO_SECURE_BOOT_KEY_FILE "${CMAKE_CURRENT_LIST_DIR}/keys/secure-boot-key.pem")
# pico_set_binary_type(non_secure_app secure)
# pico_set_binary_type(secure_app secure)
# ---------------------------------------
