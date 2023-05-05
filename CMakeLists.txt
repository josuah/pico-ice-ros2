cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_PATH ${CMAKE_CURRENT_SOURCE_DIR}/pico-sdk)
include(pico_sdk_import.cmake)

project(pico_ice_pmod C CXX ASM)

pico_sdk_init()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/pico-ice-sdk)

add_executable(pico_ice_pmod
    main.c
    usb_descriptors.c
    pmod_spi.c
    pmod_oledrgb.c
)

target_link_libraries(pico_ice_pmod
    pico_stdlib
    pico_ice_sdk
    pico_ice_usb
    pico_stdio_usb
    pico_ice_usb
    hardware_spi
)

target_include_directories(pico_ice_pmod PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
)

pico_enable_stdio_usb(pico_ice_pmod 1)
pico_enable_stdio_uart(pico_ice_pmod 0)

pico_add_extra_outputs(pico_ice_pmod)
