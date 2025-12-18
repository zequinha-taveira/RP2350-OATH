set(SECURE_APP_SOURCES
    main_secure.c
    src/secure_gateway_s.c
    src/applet_manager.c
    src/oath/oath_protocol.c
    src/oath/oath_storage.c
    src/oath/iso7816_4.c
    src/oath/openpgp_applet.c
    src/oath/openpgp_storage.c
    src/oath/fido2_applet.c
    src/oath/fido2_storage.c
    src/security/security.c
    src/security/hsm.c
    src/time_sync.c
    src/hid_keyboard.c
    src/drivers/led_driver.c
    src/crypto/aes.c
    src/crypto/aes_gcm.c
    # libcotp sources
    ../lib/libcotp/src/otp.c
    ../lib/libcotp/src/utils/base32.c
    ../lib/libcotp/src/utils/secure_zero.c
    ../lib/libcotp/src/ctx.c
    src/crypto/whmac_rp2350.c
    src/crypto/sha1.c
    src/crypto/sha256.c
    src/crypto/uECC.c
)

add_executable(secure_app ${SECURE_APP_SOURCES})

# Include directories
target_include_directories(secure_app PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/src/crypto
    ${CMAKE_CURRENT_LIST_DIR}/src/drivers
    ${CMAKE_CURRENT_LIST_DIR}/src/oath
    ${CMAKE_CURRENT_LIST_DIR}/src/security
    ${CMAKE_CURRENT_LIST_DIR}/../include
    ${CMAKE_CURRENT_LIST_DIR}/../lib
    ${CMAKE_CURRENT_LIST_DIR}/../lib/libcotp/src
    ${CMAKE_CURRENT_LIST_DIR}/../lib/libcotp/src/utils
)

# PIO configuration
pico_generate_pio_header(secure_app ${CMAKE_CURRENT_LIST_DIR}/src/drivers/ws2812.pio)

# Enable TrustZone Secure Build
set(PICO_TRUSTZONE_SECURE_BUILD 1)
target_compile_definitions(secure_app PRIVATE 
    PICO_TRUSTZONE_SECURE_BUILD=1
    uECC_SUPPORTS_secp256r1=1
    uECC_OPTIMIZATION_LEVEL=3
    uECC_SQUARE_FUNC=1
)

# Security Hardening Flags
target_compile_options(secure_app PRIVATE
    -fstack-protector-all
    -Wstack-usage=1024
)

# Linker script configuration
pico_set_linker_script(secure_app ${CMAKE_CURRENT_LIST_DIR}/memmap_secure.ld)

# Link libraries
target_link_libraries(secure_app
    pico_stdlib
    pico_multicore
    pico_rand
    hardware_flash
    hardware_sha256
    hardware_pio
    hardware_clocks
)

# Enable standard IO
pico_enable_stdio_uart(secure_app 1)
pico_enable_stdio_usb(secure_app 0)

# Generate the CMSE Import Library (NSC)
target_link_options(secure_app PRIVATE "-Wl,--cmse-implib")
target_link_options(secure_app PRIVATE "-Wl,--out-implib=${CMAKE_CURRENT_BINARY_DIR}/secure_app_import_lib.o")
