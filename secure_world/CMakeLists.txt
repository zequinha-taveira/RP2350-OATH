cmake_minimum_required(VERSION 3.13)

# Define the Secure World project
project(secure_app C CXX ASM)

# Initialize the SDK
pico_sdk_init()

# Define the target executable
add_executable(secure_app
    main_secure.c # Placeholder for your secure main file
)

# Enable TrustZone Secure Build
# This defines PICO_RP2350 and sets up appropriate compiler flags for Secure state
set(PICO_TRUSTZONE_SECURE_BUILD 1)

# Linker script configuration
# We explicitly use our custom linker script to define the secure memory layout
pico_set_linker_script(secure_app ${CMAKE_CURRENT_LIST_DIR}/memmap_secure.ld)

# Link libraries
target_link_libraries(secure_app
    pico_stdlib
    pico_multicore
    # Add other secure libraries here (e.g., hardware_flash, hardware_sha256)
)

# Enable standard IO (UART/USB) if needed for debugging the secure world
pico_enable_stdio_uart(secure_app 1)
pico_enable_stdio_usb(secure_app 0) # Usually USB is handled by NS world, but SW can have it.

# Generate the CMSE Import Library
# This library contains the symbols for Non-Secure Callable (NSC) functions
# that the Non-Secure world needs to link against.
target_link_options(secure_app PRIVATE "-Wl,--cmse-implib")
target_link_options(secure_app PRIVATE "-Wl,--out-implib=secure_app_import_lib.o")

# Add a post-build command to rename/copy the import lib to a known location if desired
# or just reference it from the build directory.
