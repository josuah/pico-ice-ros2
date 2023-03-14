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

#define SSD1331_LOCK        0xFD
#define SSD1331_LOCK_ON     (1 << 2)
#define SSD1331_LOCK_OFF    (0 << 2)

pmod_oledrgb_command(uint8_t addr, uint8_t data)
{
    pmod_spi_chip_select(PMOD_OLEDRGB_SPI_CS_N_PIN);
    pmod_spi_write_blocking(&addr, 1);
    pmod_spi_write_blocking(&data, 1);
    pmod_spi_chip_deselect(PMOD_OLEDRGB_SPI_CS_N_PIN);
}

pmod_oledrgb_gpio_init(struct pmod_oledrgb *pmod) {
    pmod_spi_init(&pmod->spi, pmod->spi_clk_pin, pmod->spi_copi_pin, pmod->spi_cipo_pin);
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

    // Enable the negative power rail
    gpio_put(pmod->vcc_en_n_pin, !true);
    gpio_put(pmod->pmod_en_pin, true);
    delay_ms(20);

    // Issue a reset pulse
    gpio_put(pmod->rst_n_pin, !true);
    delay_us(3);
    gpio_put(pmod->rst_n_pin, !false);
    delay_us(3);

    pmod_oledrgb_write();
}
