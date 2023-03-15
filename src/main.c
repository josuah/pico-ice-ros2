#include "pico/stdlib.h"
#include "pico/stdio.h"
#include "boards/pico_ice.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "ice_usb.h"
#include "ice_led.h"
#include "ice_fpga.h"
#include "ice_pmod.h"
#include "pmod_oledrgb.h"

int main(void) {
    stdio_init_all();
    tusb_init();
    pmod_spi_init();

    // Let the FPGA start and give it a clock
    ice_fpga_init(48);
    ice_fpga_start();

    for (;;) {
        tud_task();

        pmod_oledrgb_init(&ice_pmod_3);
    }
    return 0;
}
