/*
 * MIT License
 *
 * Copyright (c) 2023 tinyVision.ai
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

// https://digilent.com/reference/pmod/pmodoledrgb/reference-manual

// pico-sdk
#include "hardware/gpio.h"
#include "pico/stdlib.h"

// pico-ice-sdk
#include "ice_usb.h"

// pmod
#include "pmod.h"
#include "pmod_spi.h"
#include "pmod_oledrgb.h"

#define oledrgb_dc                      spi.io7
#define oledrgb_rst_n                   spi.io8
#define oledrgb_14v_en                  spi.io9
#define oledrgb_gnd_en                  spi.io10

#define SSD1331_COL_ADDRESS             0x15
#define SSD1331_ROW_ADDRESS             0x75
#define SSD1331_DRAW_LINE               0x21
#define SSD1331_DRAW_RECTANGLE          0x22
#define SSD1331_CLEAR_WINDOW            0x25
#define SSD1331_DEACTIVATE_SCROLLING    0x2E
#define SSD1331_CONTRAST_FOR_A          0x81
#define SSD1331_CONTRAST_FOR_B          0x82
#define SSD1331_CONTRAST_FOR_C          0x83
#define SSD1331_MASTER_CURRENT_CONTROL  0x87
#define SSD1331_PRE_CHARGE_SPEED_A      0x8A
#define SSD1331_PRE_CHARGE_SPEED_B      0x8B
#define SSD1331_PRE_CHARGE_SPEED_C      0x8C
#define SSD1331_REMAP_AND_COLOR_DEPTH   0xA0
#define SSD1331_DISPLAY_START_LINE      0xA1
#define SSD1331_DISPLAY_OFFSET          0xA2
#define SSD1331_DISPLAY_MODE_NORMAL     0xA4
#define SSD1331_DISPLAY_MODE_FULL_ON    0xA5
#define SSD1331_DISPLAY_MODE_FULL_OFF   0xA6
#define SSD1331_DISPLAY_MODE_INVERSE    0xA7
#define SSD1331_MULTIPLEX_RATIO         0xA8
#define SSD1331_DISPLAY_ON_DIM          0xAC
#define SSD1331_MASTER_CONFIG           0xAD
#define SSD1331_DISPLAY_OFF             0xAE
#define SSD1331_DISPLAY_ON_NORMAL       0xAF
#define SSD1331_POWER_SAVE_MODE         0xB0
#define SSD1331_PHASE_PERIOD_ADJUST     0xB1
#define SSD1331_CLOCK_DIV_OSC_FREQ      0xB3
#define SSD1331_PRE_CHARGE_LEVEL        0xBB
#define SSD1331_V_COMH                  0xBE
#define SSD1331_LOCK_STATE              0xFD

static void pmod_oledrgb_write(const pmod_2x_t *pmod, uint8_t const *data, size_t size) {
    pmod_spi_chip_select(&pmod->row.top, pmod->spi.cs_n);
    pmod_spi_write(&pmod->row.top, data, size);
    pmod_spi_chip_deselect(&pmod->row.top, pmod->spi.cs_n);
}

static inline void pmod_oledrgb_cmd_1(const pmod_2x_t *pmod, uint8_t a1) {
    uint8_t buf[] = { a1 };
    pmod_oledrgb_write(pmod, buf, sizeof(buf));
}

static inline void pmod_oledrgb_cmd_2(const pmod_2x_t *pmod, uint8_t a1, uint8_t a2) {
    uint8_t buf[] = { a1, a2 };
    pmod_oledrgb_write(pmod, buf, sizeof(buf));
}

static inline void pmod_oledrgb_cmd_3(const pmod_2x_t *pmod, uint8_t a1, uint8_t a2, uint8_t a3) {
    uint8_t buf[] = { a1, a2, a3 };
    pmod_oledrgb_write(pmod, buf, sizeof(buf));
}

static inline void pmod_oledrgb_cmd_5(const pmod_2x_t *pmod, uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5) {
    uint8_t buf[] = { a1, a2, a3, a4, a5 };
    pmod_oledrgb_write(pmod, buf, sizeof(buf));
}

void pmod_oledrgb_send(const pmod_2x_t *pmod, uint8_t x, uint8_t y, const uint8_t *data, size_t data_size)
{
    pmod_oledrgb_cmd_3(pmod, SSD1331_COL_ADDRESS, x, 95);
    pmod_oledrgb_cmd_3(pmod, SSD1331_ROW_ADDRESS, y, 63);
    gpio_put(pmod->oledrgb_dc, true);
    pmod_oledrgb_write(pmod, data, data_size);
    gpio_put(pmod->oledrgb_dc, false);
}

void pmod_oledrgb_init(const pmod_2x_t *pmod) {
    // spi pin init
    pmod_spi_init(&pmod->row.top);

    // pin 1 - SPI chip select (deselected)
    gpio_init(pmod->spi.cs_n);
    gpio_put(pmod->spi.cs_n, true);
    gpio_set_dir(pmod->spi.cs_n, GPIO_OUT);

    // pin 7 - data/command mode selection (command)
    gpio_init(pmod->oledrgb_dc);
    gpio_put(pmod->oledrgb_dc, false);
    gpio_set_dir(pmod->oledrgb_dc, GPIO_OUT);

    // pin 8 - SSD1331 controller reset (no reset)
    gpio_init(pmod->oledrgb_rst_n);
    gpio_put(pmod->oledrgb_rst_n, !false);
    gpio_set_dir(pmod->oledrgb_rst_n, GPIO_OUT);

    // pin 9 - 14v power rail (off)
    gpio_init(pmod->oledrgb_14v_en);
    gpio_put(pmod->oledrgb_14v_en, false);
    gpio_set_dir(pmod->oledrgb_14v_en, GPIO_OUT);

    // pin 10 - ground power rail (enable)
    gpio_init(pmod->oledrgb_gnd_en);
    gpio_put(pmod->oledrgb_gnd_en, true);
    gpio_set_dir(pmod->oledrgb_gnd_en, GPIO_OUT);
    sleep_us(20); // at least 20 us

    // Issue a reset pulse
    gpio_put(pmod->oledrgb_rst_n, !true);
    sleep_us(3); // at least 3 us
    gpio_put(pmod->oledrgb_rst_n, !false);
    sleep_us(3); // at least 3 us

    // Send the configuration
    pmod_oledrgb_cmd_2(pmod, SSD1331_LOCK_STATE, 0x12);
    pmod_oledrgb_cmd_1(pmod, SSD1331_DISPLAY_OFF);
    pmod_oledrgb_cmd_2(pmod, SSD1331_REMAP_AND_COLOR_DEPTH, 0x72);
    pmod_oledrgb_cmd_2(pmod, SSD1331_DISPLAY_START_LINE, 0);
    pmod_oledrgb_cmd_2(pmod, SSD1331_DISPLAY_OFFSET, 0);
    pmod_oledrgb_cmd_1(pmod, SSD1331_DISPLAY_MODE_NORMAL);
    pmod_oledrgb_cmd_2(pmod, SSD1331_MULTIPLEX_RATIO, 0x3F);
    pmod_oledrgb_cmd_2(pmod, SSD1331_MASTER_CONFIG, 0x8E);
    pmod_oledrgb_cmd_2(pmod, SSD1331_POWER_SAVE_MODE, 0x0B);
    pmod_oledrgb_cmd_2(pmod, SSD1331_PHASE_PERIOD_ADJUST, 0x31);
    pmod_oledrgb_cmd_2(pmod, SSD1331_CLOCK_DIV_OSC_FREQ, 0xF0);
    pmod_oledrgb_cmd_2(pmod, SSD1331_PRE_CHARGE_SPEED_A, 0x64);
    pmod_oledrgb_cmd_2(pmod, SSD1331_PRE_CHARGE_SPEED_B, 0x78);
    pmod_oledrgb_cmd_2(pmod, SSD1331_PRE_CHARGE_SPEED_C, 0x64);
    pmod_oledrgb_cmd_2(pmod, SSD1331_PRE_CHARGE_LEVEL, 0x3A);
    pmod_oledrgb_cmd_2(pmod, SSD1331_V_COMH, 0x3E);
    pmod_oledrgb_cmd_2(pmod, SSD1331_MASTER_CURRENT_CONTROL, 0x06);
    pmod_oledrgb_cmd_2(pmod, SSD1331_CONTRAST_FOR_A, 0x91);
    pmod_oledrgb_cmd_2(pmod, SSD1331_CONTRAST_FOR_B, 0x50);
    pmod_oledrgb_cmd_2(pmod, SSD1331_CONTRAST_FOR_C, 0x7D);
    pmod_oledrgb_cmd_1(pmod, SSD1331_DEACTIVATE_SCROLLING);
    pmod_oledrgb_cmd_5(pmod, SSD1331_CLEAR_WINDOW, 0, 0, 95, 63);

    // Enable the positive power rail
    gpio_put(pmod->oledrgb_14v_en, true);
    ice_usb_sleep_ms(25); // at least 25 ms

    // Turn the display on
    pmod_oledrgb_cmd_1(pmod, SSD1331_DISPLAY_ON_NORMAL);
    ice_usb_sleep_ms(100); // typical 100 ms
}
