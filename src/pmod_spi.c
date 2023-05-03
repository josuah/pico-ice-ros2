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

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>
#include "hardware/sync.h"
#include "pico/stdlib.h"
#include "boards/pico_ice.h"
#include "pmod_spi.h"
#include "pmod.h"

void pmod_spi_init(const pmod_1x_t *pmod) {
    gpio_init(pmod->spi.cipo);

    gpio_init(pmod->spi.copi);
    gpio_put(pmod->spi.copi, true);
    gpio_set_dir(pmod->spi.copi, GPIO_OUT);

    gpio_init(pmod->spi.clk);
    gpio_put(pmod->spi.clk, true);
    gpio_set_dir(pmod->spi.clk, GPIO_OUT);
}

static void delay(void)
{
    sleep_us(100);
}

static uint8_t transfer_byte(const pmod_1x_t *pmod, uint8_t tx) {
    uint8_t rx;

    for (uint8_t i = 0; i < 8; i++) {
        // Update TX and immediately set negative edge.
        gpio_put(pmod->spi.clk, false);
        gpio_put(pmod->spi.copi, tx >> 7);
        tx <<= 1;

        // stable for a while with clock low
        delay();

        // Sample RX as we set positive edge.
        rx <<= 1;
        rx |= gpio_get(pmod->spi.cipo) & 1;
        gpio_put(pmod->spi.clk, true);

        // stable for a while with clock high
        delay();
    }
    return rx;
}

void pmod_spi_chip_select(const pmod_1x_t *pmod, uint8_t cs_n_pin) {
    gpio_put(cs_n_pin, false);
    gpio_set_dir(cs_n_pin, GPIO_OUT);
    delay();
}

void pmod_spi_chip_deselect(const pmod_1x_t *pmod, uint8_t cs_n_pin) {
    gpio_put(pmod->spi.copi, true);
    gpio_put(cs_n_pin, true);
    gpio_set_dir(cs_n_pin, GPIO_OUT);
    delay();
}

void pmod_spi_write(const pmod_1x_t *pmod, uint8_t const *buf, size_t len) {
    for (; len > 0; len--, buf++) {
        transfer_byte(pmod, *buf);
    }
}

void pmod_spi_read(const pmod_1x_t *pmod, uint8_t tx, uint8_t *buf, size_t len) {
    for (; len > 0; len--, buf++) {
        *buf = transfer_byte(pmod, tx);
    }
}
