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
#include "pmod_spi.h"
#include "pmod_oledrgb.h"

#define DC_PIN_DATA
#define DC_PIN_COMMAND

#define SSD1331_LOCK                    0xFD
#define SSD1331_REMAP_AND_COLOR_DEPTH   0xA0
#define SSD1331_DISPLAY_START_LINE      0xA1
#define SSD1331_DISPLAY_OFFSET          0xA2
#define SSD1331_DISPLAY_ON_DIM          0xAC
#define SSD1331_DISPLAY_ON_NORMAL       0xAF
#define SSD1331_DISPLAY_OFF             0xAE
#define SSD1331_DISPLAY_MODE_NORMAL     0xA4
#define SSD1331_DISPLAY_MODE_FULL_ON    0xA5
#define SSD1331_DISPLAY_MODE_FULL_OFF   0xA6
#define SSD1331_DISPLAY_MODE_INVERSE    0xA7
#define SSD1331_MULTIPLEX_RATIO         0xA8
#define SSD1331_MASTER_CONFIG           0xAD
#define SSD1331_POWER_SAVE_MODE         0xB0
#define SSD1331_PHASE_PERIOD_ADJUST     0xB1
#define SSD1331_CLOCK_DIV_OSC_FREQ      0xB3
#define SSD1331_PRE_CHARGE_SPEED_A      0x8A
#define SSD1331_PRE_CHARGE_SPEED_B      0x8B
#define SSD1331_PRE_CHARGE_SPEED_C      0x8C
#define SSD1331_PRE_CHARGE_LEVEL        0xBB
#define SSD1331_V_COMH                  0xBE
#define SSD1331_MASTER_CURRENT_CONTROL  0x87
#define SSD1331_CONTRAST_FOR_A          0x81
#define SSD1331_CONTRAST_FOR_B          0x82
#define SSD1331_CONTRAST_FOR_C          0x83
#define SSD1331_DEACTIVATE_SCROLLING    0x2E
#define SSD1331_CLEAR_WINDOW            0x25
#define SSD1331_DRAW_LINE               0x21

#define OP1(addr)                       1, addr
#define OP2(addr, value)                2, addr, value
#define OP5(addr, v1,v2,v3,v4)          5, addr, v1,v2,v3,v4
#define OP8(addr, v1,v2,v3,v4,v5,v6,v7) 8, addr, v1,v2,v3,v4,v5,v6,v7
#define END                             0

static const uint8_t config_opcode[] = {
    OP2(SSD1331_LOCK, 0x12),
    OP1(SSD1331_DISPLAY_OFF),
    OP2(SSD1331_REMAP_AND_COLOR_DEPTH, 0x72),
    OP2(SSD1331_DISPLAY_START_LINE, 0),
    OP2(SSD1331_DISPLAY_OFFSET, 0),
    OP1(SSD1331_DISPLAY_MODE_NORMAL),
    OP2(SSD1331_MULTIPLEX_RATIO, 0x3F),
    OP2(SSD1331_MASTER_CONFIG, 0x8E),
    OP2(SSD1331_POWER_SAVE_MODE, 0B),
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

static const uint8_t turn_on_opcode[] = {
    OP1(SSD1331_DISPLAY_ON),
    END
}

static const uint8_t test_draw_opcode[] = {
    OP7(DRAW_LINE, 0, 0, 30, 30, 35,0,0)
    END
}

static void pmod_oledrgb_write(struct pmod_oledrgb *pmod, size_t *data, size_t size) {
    pmod_spi_chip_select(&pmod->spi, PMOD_OLEDRGB_SPI_CS_N_PIN);
    pmod_spi_write_blocking(&pmod->spi, &addr, 1);
    pmod_spi_write_blocking(&pmod->spi, &data, 1);
    pmod_spi_chip_deselect(&pmod->spi, PMOD_OLEDRGB_SPI_CS_N_PIN);
}

static void pmod_oledrgb_run(struct pmod_oledrgb *pmod, uint8_t const *opcode) {
    for (uint8_t *pos = config_opcode; *pos != 0; pos += 1 + *pos) {
        pmod_oledrgb_write(pmod, pos + 1, *pos);
    }
}

static void pmod_oledrgb_gpio_init(struct pmod_oledrgb *pmod) {
    pmod_spi_init(&pmod->spi);
    gpio_init(pmod->spi_cs_n_pin);

    gpio_init(pmod->dc_pin);
    gpio_set_dir(pmod->dc_pin, GPIO_OUT);

    gpio_init(pmod->rst_n_pin);
    gpio_set_dir(pmod->rst_n_pin, GPIO_OUT);

    gpio_init(pmod->vcc_en_n_pin);
    gpio_set_dir(pmod->vcc_en_n_pin, GPIO_OUT);

    gpio_init(pmod->pmod_en_pin);
    gpio_set_dir(pmod->pmod_en_pin, GPIO_OUT);

    // initial state
    gpio_put(pmod->dc_pin, DC_PIN_XXXX);
    gpio_put(pmod->rst_n_pin, !false);

    // Enable the negative and positive power rail
    gpio_put(pmod->vcc_en_n_pin, !true);
    gpio_put(pmod->pmod_en_pin, true);
    delay_ms(20);

    // Issue a reset pulse
    gpio_put(pmod->rst_n_pin, !true);
    delay_us(3);
    gpio_put(pmod->rst_n_pin, !false);
    delay_us(3);

    // Send the configuration
    pmod_oledrgb_run(config_opcode);

    // Pause the positive power rail
    gpio_put(pmod->vcc_en_n_pin, !true);
    sleep_ms(25);

    // Turn the display on
    pmod_oledrgb_run(turn_on_opcode);
    sleep_ms(100);

    // Draw something to test
    // TODO debug
    pmod_oledrgb_run(test_draw_opcode);
}
