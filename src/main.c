#include "pico/stdlib.h"
#include "pico/stdio.h"
#include "boards/pico_ice.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "ice_usb.h"
#include "ice_led.h"
#include "ice_fpga.h"

#define FPGA_SPI_CIPO_PIN   0
#define FPGA_SPI_CS_N_PIN   1
#define FPGA_SPI_CLK_PIN    2
#define FPGA_SPI_COPI_PIN   3

static void fpga_spi_init(void) {
    spi_init(spi0, 48000);

    gpio_set_dir(FPGA_SPI_CS_N_PIN, GPIO_OUT);
    gpio_set_function(FPGA_SPI_CS_N_PIN, GPIO_FUNC_SIO);

    gpio_set_function(FPGA_SPI_CLK_PIN,  GPIO_FUNC_SPI);
    gpio_set_function(FPGA_SPI_CIPO_PIN, GPIO_FUNC_SPI);
    gpio_set_function(FPGA_SPI_COPI_PIN, GPIO_FUNC_SPI);
}

static void fpga_spi_chip_select(void) {
    gpio_put(FPGA_SPI_CS_N_PIN, false);
}

static void fpga_spi_chip_deselect(void) {
    gpio_put(FPGA_SPI_CS_N_PIN, true);
}

static void fpga_spi_write(uint8_t *data, size_t size) {
    spi_write_blocking(spi0, data, size);
}

int main(void) {
    stdio_init_all();
    tusb_init();
    fpga_spi_init();

    // Let the FPGA start and give it a clock
    ice_fpga_init(48);
    ice_fpga_start();

    for (;;) {
        tud_task();

        fpga_spi_chip_select();
        fpga_spi_write("\x55\x55\x55\x55", 4);
        fpga_spi_chip_deselect();

        sleep_us(100);
    }
    return 0;
}
