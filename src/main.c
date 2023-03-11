#include "pico/stdlib.h"
#include "pico/stdio.h"
#include "boards/pico_ice.h"
#include "ice_usb.h"
#include "ice_led.h"
#include "ice_spi.h"

int main(void) {
    stdio_init_all();
    ice_led_init();
    ice_spi_init();

    for (;;) {
        tud_task();

        ice_spi_chip_select(ICE_FPGA_SPI_CSN_PIN);
        ice_spi_write_blocking("\x55\x55\x55\x55", 4);
        ice_spi_chip_deselect(ICE_FPGA_SPI_CSN_PIN);
    }
    return 0;
}
