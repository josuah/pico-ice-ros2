cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_PATH ${CMAKE_CURRENT_SOURCE_DIR}/lib/pico-sdk)
include(pico_sdk_import.cmake)

project(pico_ice_pmod C CXX ASM)

pico_sdk_init()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/lib/pico-ice-sdk)
link_directories(${CMAKE_CURRENT_LIST_DIR}/lib/micro_ros_raspberrypi_pico_sdk/libmicroros)

add_executable(pico_ice_pmod
    lib/micro_ros_raspberrypi_pico_sdk/pico_uart_transport.c
    src/main.c
    src/usb_descriptors.c
    src/pmod_spi.c
    src/pmod_oledrgb.c
)

target_link_libraries(pico_ice_pmod
    pico_stdlib
    pico_ice_sdk
    pico_ice_usb
    pico_stdio_usb
    pico_ice_usb
    hardware_spi
    microros
)

target_include_directories(pico_ice_pmod PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/lib/micro_ros_raspberrypi_pico_sdk
    ${CMAKE_CURRENT_LIST_DIR}/lib/micro_ros_raspberrypi_pico_sdk/libmicroros/include
)

pico_enable_stdio_usb(pico_ice_pmod 1)
pico_enable_stdio_uart(pico_ice_pmod 0)
add_compile_definitions(PICO_UART_ENABLE_CRLF_SUPPORT=0)
add_compile_definitions(PICO_STDIO_ENABLE_CRLF_SUPPORT=0)
add_compile_definitions(PICO_STDIO_DEFAULT_CRLF=0)

pico_add_extra_outputs(pico_ice_pmod)
