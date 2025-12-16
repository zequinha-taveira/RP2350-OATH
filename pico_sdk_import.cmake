# This is a shim to allow including pico_sdk_init.cmake from the external SDK location
set(PICO_SDK_PATH "../../pico-sdk" CACHE PATH "Path to the Raspberry Pi Pico SDK")
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)
