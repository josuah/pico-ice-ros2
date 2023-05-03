// libc
#include <stdio.h>

// pico-sdk
#include "pico/stdlib.h"
#include "boards/pico_ice.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"

// pico-ice
#include "ice_usb.h"
#include "ice_led.h"
#include "ice_fpga.h"
#include "ice_pmod.h"
#include "pmod_oledrgb.h"
#include "pmod_spi.h"

int main(void)
{
    tusb_init();
    stdio_usb_init();

    // Let the FPGA start and give it a clock
    ice_fpga_init(48);
    ice_fpga_start();

    // PMOD init
    pmod_oledrgb_init(&ice_pmod_4);

    while (1) {
        tud_task();
    }

    return 0;
}
