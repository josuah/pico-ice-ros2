cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/pico-sdk)
include(pico_sdk_import.cmake)

project(pico_ice_pmod_demo C CXX ASM)

pico_sdk_init()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/pico-ice-sdk)

add_executable(pico_ice_pmod_demo
    src/main.c
    src/usb_descriptors.c
    src/pmod_spi.c
)
target_link_libraries(pico_ice_pmod_demo
    pico_ice_sdk
    pico_ice_usb
    pico_stdio_usb
)
target_include_directories(pico_ice_pmod_demo PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)
pico_enable_stdio_usb(pico_ice_pmod_demo 1)
pico_add_extra_outputs(pico_ice_pmod_demo)
