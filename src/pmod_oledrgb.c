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

#include "hardware/gpio.h"
#include "pico/stdlib.h"
#include "ice_usb.h"
#include "pmod.h"
#include "pmod_spi.h"
#include "pmod_oledrgb.h"

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

#define OP1(addr)                       1, addr
#define OP2(addr, v1)                   2, addr, v1
#define OP3(addr, v1,v2)                3, addr, v1,v2
#define OP5(addr, v1,v2,v3,v4)          5, addr, v1,v2,v3,v4
#define OP8(addr, v1,v2,v3,v4,v5,v6,v7) 8, addr, v1,v2,v3,v4,v5,v6,v7
#define OP11(addr, v1,v2,v3,v4,v5,v6,v7,v8,v9,v10)  11, addr, v1,v2,v3,v4,v5,v6,v7,v8,v9,v10
#define END                             0

static void pmod_oledrgb_write(const pmod_2x_t *pmod, uint8_t const *data, size_t size) {
    pmod_spi_chip_select(&pmod->row.top, pmod->spi.cs_n);
    pmod_spi_write(&pmod->row.top, data, size);
    pmod_spi_chip_deselect(&pmod->row.top, pmod->spi.cs_n);
}

static void pmod_oledrgb_run(const pmod_2x_t *pmod, uint8_t const *opcode) {
    for (uint8_t const *pos = opcode; *pos != 0; pos += 1 + *pos) {
        pmod_oledrgb_write(pmod, pos + 1, *pos);
    }
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
    ice_usb_sleep_ms(1); // at least 20 us

    // Issue a reset pulse
    gpio_put(pmod->oledrgb_rst_n, !true);
    ice_usb_sleep_ms(1); // at least 3 us
    gpio_put(pmod->oledrgb_rst_n, !false);
    ice_usb_sleep_ms(1); // at least 3 us

    // Send the configuration
    static const uint8_t config_opcode[] = {
        //OP2(SSD1331_LOCK_STATE, 0x12),
        OP1(SSD1331_DISPLAY_OFF),
        OP2(SSD1331_REMAP_AND_COLOR_DEPTH, 0x72),
        OP2(SSD1331_DISPLAY_START_LINE, 0),
        OP2(SSD1331_DISPLAY_OFFSET, 0),
        OP1(SSD1331_DISPLAY_MODE_NORMAL),
        OP2(SSD1331_MULTIPLEX_RATIO, 0x3F),
        OP2(SSD1331_MASTER_CONFIG, 0x8E),
        OP2(SSD1331_POWER_SAVE_MODE, 0x0B),
        OP2(SSD1331_PHASE_PERIOD_ADJUST, 0x31),
        OP2(SSD1331_CLOCK_DIV_OSC_FREQ, 0xF0),
        OP2(SSD1331_PRE_CHARGE_SPEED_A, 0x64),
        OP2(SSD1331_PRE_CHARGE_SPEED_B, 0x78),
        OP2(SSD1331_PRE_CHARGE_SPEED_C, 0x64),
        OP2(SSD1331_PRE_CHARGE_LEVEL, 0x3A),
        OP2(SSD1331_V_COMH, 0x3E),
        OP2(SSD1331_MASTER_CURRENT_CONTROL, 0x06),
        OP2(SSD1331_CONTRAST_FOR_A, 0x91),
        OP2(SSD1331_CONTRAST_FOR_B, 0x50),
        OP2(SSD1331_CONTRAST_FOR_C, 0x7D),
        OP1(SSD1331_DEACTIVATE_SCROLLING),
        OP5(SSD1331_CLEAR_WINDOW, 0, 0, 95, 63),
        END
    };
    //pmod_oledrgb_run(pmod, config_opcode);

    // Enable the positive power rail
    gpio_put(pmod->oledrgb_14v_en, true);
    ice_usb_sleep_ms(200); // at least 100 ms

    // Turn the display on
    static const uint8_t turn_on_opcode[] = {
        OP1(SSD1331_DISPLAY_ON_NORMAL),
        END
    };
    //pmod_oledrgb_run(pmod, turn_on_opcode);
    ice_usb_sleep_ms(300); // at least 25 ms

    static const uint8_t test_opcode[] = {
        OP3(SSD1331_COL_ADDRESS, 10, 95),
        OP3(SSD1331_ROW_ADDRESS, 15, 63)
    };
    pmod_oledrgb_run(pmod, test_opcode);
    ice_usb_sleep_ms(5);

    static const uint8_t pixel_data[] = {
        0x0F,0xFF
    };
    gpio_put(pmod->oledrgb_dc, true);
    pmod_oledrgb_write(pmod, pixel_data, sizeof(pixel_data));
    gpio_put(pmod->oledrgb_dc, false);
}
