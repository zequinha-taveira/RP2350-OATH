cmake_minimum_required(VERSION 3.13)

# Define the Secure World project
project(secure_app C CXX ASM)

# Initialize the SDK
pico_sdk_init()

# Define the target executable
add_executable(secure_app
    main_secure.c
    # Include other source files from src/
    src/secure_gateway_s.c
    src/oath/oath_protocol.c
    src/oath/oath_storage.c
    src/security/security.c
    # src/crypto/hmac.c - Removed, using libcotp/mbedtls
    # src/main_s.c - Merged into main_secure.c
    # libcotp sources
    ../lib/libcotp/src/otp.c
    ../lib/libcotp/src/utils/base32.c
    ../lib/libcotp/src/utils/secure_zero.c
    ../lib/libcotp/src/ctx.c
    # ../lib/libcotp/src/utils/whmac_mbedtls.c # Use custom backend
    src/crypto/whmac_rp2350.c
    src/crypto/sha1.c
    src/crypto/sha256.c
    # Add other sources as needed recursively or explicitly
)

# Include directories
target_include_directories(secure_app PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/src/crypto
    ${CMAKE_CURRENT_LIST_DIR}/src/oath
    ${CMAKE_CURRENT_LIST_DIR}/src/security
    ${CMAKE_CURRENT_LIST_DIR}/../include
    ${CMAKE_CURRENT_LIST_DIR}/../lib
    ${CMAKE_CURRENT_LIST_DIR}/../lib/libcotp/src
)

# Enable TrustZone Secure Build
set(PICO_TRUSTZONE_SECURE_BUILD 1)
target_compile_definitions(secure_app PRIVATE PICO_TRUSTZONE_SECURE_BUILD=1)

# Linker script configuration
pico_set_linker_script(secure_app ${CMAKE_CURRENT_LIST_DIR}/memmap_secure.ld)

# Link libraries
target_link_libraries(secure_app
    pico_stdlib
    pico_multicore
    hardware_flash
    hardware_sha256
    # pico_mbedtls # Not used
)

# Enable standard IO
pico_enable_stdio_uart(secure_app 1)
pico_enable_stdio_usb(secure_app 0)

# Generate the CMSE Import Library (NSC)
target_link_options(secure_app PRIVATE "-Wl,--cmse-implib")
target_link_options(secure_app PRIVATE "-Wl,--out-implib=${CMAKE_CURRENT_BINARY_DIR}/secure_app_import_lib.o")
