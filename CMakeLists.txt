# CMakeLists.txt for rp2350-oath

cmake_minimum_required(VERSION 3.13)

# Pull in Raspberry Pi Pico SDK
# Download from https://github.com/raspberrypi/pico-sdk
set(PICO_SDK_PATH "../../pico-sdk" CACHE PATH "Path to the Raspberry Pi Pico SDK")
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

project(rp2350_oath C CXX ASM)

# Initialize the SDK
pico_sdk_init()

# Add libcotp as a subdirectory
add_subdirectory(lib/libcotp)

# Add executable
add_executable(rp2350_oath
    src/main.c
    src/usb/usb_descriptors.c
    src/usb/ccid_device.c
    src/oath/oath_protocol.c
    src/oath/oath_storage.c
    src/crypto/hmac.c
    src/ui/led_feedback.c
    src/ui/button_handler.c
)

# Include directories
target_include_directories(rp2350_oath PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/src
    ${CMAKE_CURRENT_LIST_DIR}/src/usb
    ${CMAKE_CURRENT_LIST_DIR}/src/oath
    ${CMAKE_CURRENT_LIST_DIR}/src/crypto
    ${CMAKE_CURRENT_LIST_DIR}/src/ui
)

# Link libraries
target_link_libraries(rp2350_oath
    pico_stdlib
    hardware_flash
    hardware_sync
    tinyusb_device
    cotp
    pico_sha256_hw
)

# Enable USB and STDIO over USB
pico_enable_stdio_usb(rp2350_oath 1)

# TinyUSB configuration
pico_set_program_link_dirs(rp2350_oath ${CMAKE_CURRENT_LIST_DIR}/src)
pico_set_program_compile_definitions(rp2350_oath TUSB_CONFIG_FILE=tusb_config.h)
pico_enable_stdio_uart(rp2350_oath 0)

# Add extra outputs (UF2 file)
pico_add_extra_outputs(rp2350_oath)
