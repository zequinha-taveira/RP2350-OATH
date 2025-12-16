cmake_minimum_required(VERSION 3.13)

# Initialize SDK
set(PICO_TINYUSB_PATH ${CMAKE_CURRENT_LIST_DIR}/lib/tinyusb)
include(pico_sdk_import.cmake)

project(rp2350_oath_trustzone C CXX ASM)

# Initialize the SDK
pico_sdk_init()

# Add Secure World
add_subdirectory(secure_world)

# Use a specific variable to pass the import lib location to the Non-Secure World
# The secure_world CMakeLists.txt must produce this file.
set(SECURE_IMPORT_LIB ${CMAKE_CURRENT_BINARY_DIR}/secure_world/secure_app_import_lib.o)

# Add Non-Secure World
# We pass the SECURE_IMPORT_LIB variable to the child scope automatically, 
# but we can also be explicit if needed.
add_subdirectory(non_secure_world)
