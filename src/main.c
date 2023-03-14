#include "pico/stdlib.h"
#include "pico/stdio.h"
#include "boards/pico_ice.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "ice_usb.h"
#include "ice_led.h"
#include "ice_fpga.h"
#include "pmod_spi.h"

int main(void) {
    stdio_init_all();
    tusb_init();
    pmod_spi_init();

    // Configure bare GPIOs
    gpio_init(ICE_PMOD2A_SPI_CS_PIN);

    // Let the FPGA start and give it a clock
    ice_fpga_init(48);
    ice_fpga_start();

    for (;;) {
        tud_task();

        pmod_spi_chip_select(ICE_PMOD2A_SPI_CS_PIN);
        pmod_spi_write_blocking("\x12\x34\x56\x78", 4);
        pmod_spi_chip_deselect(ICE_PMOD2A_SPI_CS_PIN);
    }
    return 0;
}
