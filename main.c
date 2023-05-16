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

#include "font.h"

uint8_t framebuffer[64][96][2];

uint32_t g_imu_x = 0;
uint32_t g_imu_y = 0;
uint32_t g_imu_z = 0;

void draw_flush(void) {
    pmod_oledrgb_send(&ice_pmod_4, 0, 0, (uint8_t *)framebuffer, sizeof(framebuffer));
}

void draw_char(uint16_t x, uint16_t y, uint8_t c) {
    for (uint8_t gy = 0; gy < GLYPH_HEIGHT; gy++) {
        for (uint8_t gx = 0; gx < GLYPH_WIDTH; gx++) {
            framebuffer[63 - (x + gx)][y + gy][0] = font[c][gy][gx * 2 + 0];
            framebuffer[63 - (x + gx)][y + gy][1] = font[c][gy][gx * 2 + 1];
        }
    }
}

void draw_text(uint16_t x, uint16_t y, char *s) {
    for (size_t i = 0; s[i] != '\0'; i++) {
        draw_char(x + i * (GLYPH_WIDTH + 1), y, s[i]);
    }
}

void ice_wishbone_serial_read_cb(uint32_t addr, uint8_t *data, size_t size) {
    printf("read addr=0x%08lx size=x%d\r\n", addr, size);
}

void draw_label_value(uint16_t x, uint16_t y, char *label, uint32_t value) {
    char text[30];

    snprintf(text, sizeof(text), "%s: %ld.%03ld", label, value  / 1000, value % 1000);
    draw_text(5, y, text); y += 14;
}

void ice_wishbone_serial_write_cb(uint32_t addr, const uint8_t *data, size_t size) {
    printf("write addr=0x%08lx size=x%d data=0x", addr, size);
    for (size_t i = 0; i < size; i++) {
        printf("%02X", data[i]);
    }
    printf("\r\n");

    uint32_t u32 = data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3] << 0;
    uint16_t y = 0;

    switch (addr) {
    case 0x1000:
        g_imu_x = u32;
        break;
    case 0x1001:
        g_imu_y = u32;
        break;
    case 0x1002:
        g_imu_z = u32;
        break;
    }

    draw_text(0, y, "ROS input"); y += 18;
    draw_text(0, y, "IMU angle:"); y += 18;
    y += 4;
    draw_label_value(5, y, "x", g_imu_x); y += 14;
    draw_label_value(5, y, "y", g_imu_y); y += 14;
    draw_label_value(5, y, "z", g_imu_z); y += 14;
    draw_flush();
}

void ice_wishbone_serial_tx_cb(uint8_t byte) {
    tud_cdc_n_write_char(1, byte);
    tud_cdc_n_write_flush(1);
}

int main(void) {
    tusb_init();
    stdio_usb_init();

    // custom wishbone USB handler
    ice_usb_cdc_table[1] = &ice_wishbone_serial;

    // Let the FPGA start and give it a clock
    ice_fpga_init(48);

    // PMOD init
    pmod_oledrgb_init(&ice_pmod_4);

    uint16_t y = 0;
    draw_text(0, y, "ROS input"); y += 18;
    draw_flush();

    while (1) {
        tud_task();
    }

    return 0;
}
