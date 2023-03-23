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

    // Let the FPGA start and give it a clock
    ice_fpga_init(48);
    ice_fpga_start();

    gpio_init(ice_pmod_3.gpio.io1);
    gpio_init(ice_pmod_3.gpio.io2);
    gpio_init(ice_pmod_3.gpio.io3);
    gpio_init(ice_pmod_3.gpio.io4);

    // SPI bus configuration
    gpio_set_function(ice_pmod_3.gpio.io7, GPIO_FUNC_SPI);
    gpio_set_function(ice_pmod_3.gpio.io8, GPIO_FUNC_SPI);
    gpio_set_function(ice_pmod_3.gpio.io9, GPIO_FUNC_SPI);
    gpio_set_function(ice_pmod_3.gpio.io10, GPIO_FUNC_SPI);
    gpio_set_dir(ice_pmod_3.gpio.io7, GPIO_IN);
    gpio_set_dir(ice_pmod_3.gpio.io8, GPIO_OUT);
    gpio_set_dir(ice_pmod_3.gpio.io9, GPIO_OUT);
    gpio_set_dir(ice_pmod_3.gpio.io10, GPIO_OUT);
    spi_init(spi0, 100000);

    while (1) {
        uint8_t byte;

        tud_task();
        spi_read_blocking(spi0, 0, &byte, 1);

        switch (byte) {
        case 0:
            break;
        default:
            printf("unknown command: 0x%02X\r\n", byte);
        }
    }

    return 0;
}
