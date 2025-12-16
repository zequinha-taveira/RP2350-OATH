cmake_minimum_required(VERSION 3.13)

# Define the Non-Secure World project
project(non_secure_app C CXX ASM)

# Initialize the SDK
pico_sdk_init()

# Define the target executable
add_executable(non_secure_app
    main_ns.c # Placeholder
)

# Linker script configuration
# Explicitly use the Non-Secure memory map
pico_set_linker_script(non_secure_app ${CMAKE_CURRENT_LIST_DIR}/memmap_ns.ld)

# Link libraries
target_link_libraries(non_secure_app
    pico_stdlib
    pico_multicore
    # Add other libraries
)

# IMPORTANT: Link against the Secure World Import Library
# This allows the NS world to call functions exported by the SW.
# The `SECURE_IMPORT_LIB` variable should be passed from the parent CMake or defined via command line.
if (DEFINED SECURE_IMPORT_LIB)
    target_link_libraries(non_secure_app ${SECURE_IMPORT_LIB})
else()
    message(WARNING "SECURE_IMPORT_LIB not defined. Non-Secure app properly won't be able to call Secure functions.")
endif()

# Enable STDIO
pico_enable_stdio_uart(non_secure_app 1)
pico_enable_stdio_usb(non_secure_app 1)
