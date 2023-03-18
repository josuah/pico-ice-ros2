#include "pico/stdlib.h"
#include "pico/stdio.h"
#include "boards/pico_ice.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "ice_usb.h"
#include "ice_led.h"
#include "ice_fpga.h"
#include "ice_pmod.h"
#include "pmod_oledrgb.h"
#include "pmod_spi.h"

int main(void) {
    tusb_init();
    stdio_usb_init();

    // temporary for debug
    for (uint8_t i = 0; i <= 29; i++)
        gpio_init(i);

    // Let the FPGA start and give it a clock
    ice_fpga_init(48);
    ice_fpga_start();

    while (1) {
        tud_task();
    }

    return 0;
}
