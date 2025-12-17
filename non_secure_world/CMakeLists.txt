cmake_minimum_required(VERSION 3.13)

# Define the Non-Secure World project
project(non_secure_app C CXX ASM)

# Initialize the SDK
pico_sdk_init()

# Define the target executable
add_executable(non_secure_app
    src/main.c
    src/usb_descriptors.c
    src/usb/ccid_device.c
    src/usb/fido2_device.c
    src/usb/fido2_ctap21.c
    src/usb/fido2_bioenrollment.c
    src/usb/webusb_device.c
    src/usb/webusb_crypto.c
    src/usb/usb_composite.c
    src/secure_gateway.c
)

# Include directories
target_include_directories(non_secure_app PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/../include
    ${PICO_TINYUSB_PATH}/src
)

# Linker script configuration
pico_set_linker_script(non_secure_app ${CMAKE_CURRENT_LIST_DIR}/memmap_ns.ld)

# Link libraries
# Link libraries
target_link_libraries(non_secure_app
    pico_stdlib
    pico_multicore
    hardware_flash
    tinyusb_device
    tinyusb_board
)

# Link against the Secure World Import Library
if (DEFINED SECURE_IMPORT_LIB)
    # Ninja/CMake restriction: Can't easily define a rule for a file in another directory 
    # from here using add_custom_command. So we copy it to our binary dir.
    add_dependencies(non_secure_app secure_app)
    # Use link_options to avoid Ninja complaining about missing rule for the file
    target_link_options(non_secure_app PRIVATE ${SECURE_IMPORT_LIB})
else()
    message(WARNING "SECURE_IMPORT_LIB not defined. Non-Secure app won't be able to call Secure functions.")
endif()

# Enable STDIO
pico_enable_stdio_uart(non_secure_app 1)
pico_enable_stdio_usb(non_secure_app 1) # NS world handles USB

# TinyUSB config
# target_include_directories is already set for src/
# target_compile_definitions(non_secure_app PRIVATE TUSB_CONFIG_FILE=tusb_config.h)

# Add UF2 output for the NS world (which works as the main image effectively if combined, or we load both)
# Actually, for TrustZone, we often produce a combined UF2 or load them separately.
pico_add_extra_outputs(non_secure_app)
