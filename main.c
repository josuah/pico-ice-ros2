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
#include "ice_wishbone.h"
#include "ice_pmod.h"

// local
#include "pmod_oledrgb.h"
#include "pmod_spi.h"

void ice_wishbone_serial_read_cb(uint32_t addr, uint8_t *data, size_t size) {
    printf("0x%08lx [x%d]\r\n", addr, size);
}

void ice_wishbone_serial_write_cb(uint32_t addr, const uint8_t *data, size_t size) {
    printf("0x%08lx [x%d] 0x", addr, size);
    for (size_t i = 0; i < size; i++) {
        printf("%02X", data[i]);
    }
    printf("\r\n");
}

void ice_wishbone_serial_tx_cb(uint8_t byte) {
    tud_cdc_n_write_char(1, byte);
    tud_cdc_n_write_flush(1);
}

int main(void)
{
#define X 0xFF,0xFF
#define _ 0x00,0x00
    static const uint8_t pixel_data[] = {
        X,_,X,_,X,_,X,_,X,_,X,_,X,_,X,_,
        X,_,X,_,X,_,X,_,X,_,X,_,X,_,X,_,
        X,_,X,_,X,_,X,_,X,_,X,_,X,_,X,_,
        X,_,X,_,X,_,X,_,X,_,X,_,X,_,X,_,
    };

    tusb_init();
    stdio_usb_init();

    // custom wishbone USB handler
    //ice_usb_cdc_table[1] = &debug;

    // Let the FPGA start and give it a clock
    ice_fpga_init(48);

    // PMOD init
    pmod_oledrgb_init(&ice_pmod_4);
    pmod_oledrgb_send(&ice_pmod_4, 10, 63, pixel_data, sizeof(pixel_data));

    while (1) {
        tud_task();
    }

    return 0;
}
