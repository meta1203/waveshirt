# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)

project(waveshirt C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
add_executable(waveshirt
arduinoFFT/arduinoFFT.cpp
arduinoFFT/arduinoFFT.h
arduinoFFT/defs.h
arduinoFFT/types.h
arduinoFFT/enumsFFT.h

inmp441/inmp441.h
inmp441/inmp441.cpp

audio_analyzer.h
audio_analyzer.cpp
ws2812.h
ws2812.cpp
waveshirt.cpp
)

pico_set_program_name(waveshirt "waveshirt")
pico_set_program_version(waveshirt "0.1")

# Generate PIO header
pico_generate_pio_header(waveshirt ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio)
pico_generate_pio_header(waveshirt ${CMAKE_CURRENT_LIST_DIR}/inmp441/inmp441.pio)

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(waveshirt 1)
pico_enable_stdio_usb(waveshirt 0)

# Add the standard library to the build
target_link_libraries(waveshirt
        pico_stdlib)

# Add the standard include files to the build
target_include_directories(waveshirt PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

# Add any user requested libraries
target_link_libraries(waveshirt 
        hardware_pio
        pico_stdlib
        pico_multicore
        pico_sync
        pico_audio_i2s
        )

pico_add_extra_outputs(waveshirt)

