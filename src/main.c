#include "pico/stdlib.h"
#include "pico/stdio.h"
#include "boards/pico_ice.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "ice_usb.h"
#include "ice_led.h"
#include "ice_fpga.h"
#include "ice_pmod.h"
#include "pmod_oledrgb.h"
#include "pmod_spi.h"

#define REMOTE_KEY_VOL_MINUS    0x00
#define REMOTE_KEY_PLAY_PAUSE   0x80
#define REMOTE_KEY_VOL_PLUS     0x40
#define REMOTE_KEY_SETUP        0x20
#define REMOTE_KEY_UP           0xA0
#define REMOTE_KEY_STOP_MODE    0x60
#define REMOTE_KEY_LEFT         0x10
#define REMOTE_KEY_ENTER        0x90
#define REMOTE_KEY_RIGHT        0x50
#define REMOTE_KEY_NUM_0        0x30
#define REMOTE_KEY_BOTTOM       0xB0
#define REMOTE_KEY_RETURN       0x70
#define REMOTE_KEY_NUM_1        0x08
#define REMOTE_KEY_NUM_2        0x88
#define REMOTE_KEY_NUM_3        0x48
#define REMOTE_KEY_NUM_4        0x28
#define REMOTE_KEY_NUM_5        0xA8
#define REMOTE_KEY_NUM_6        0x68
#define REMOTE_KEY_NUM_7        0x18
#define REMOTE_KEY_NUM_8        0x98
#define REMOTE_KEY_NUM_9        0x58

int main(void) {
    tusb_init();
    stdio_usb_init();

    // Let the FPGA start and give it a clock
    ice_fpga_init(48);
    ice_fpga_start();

    gpio_init(ice_pmod_3.gpio.io1);
    gpio_init(ice_pmod_3.gpio.io2);
    gpio_init(ice_pmod_3.gpio.io3);
    gpio_init(ice_pmod_3.gpio.io4);

    // SPI bus configuration
    gpio_set_function(ice_pmod_3.gpio.io7, GPIO_FUNC_SPI);
    gpio_set_function(ice_pmod_3.gpio.io8, GPIO_FUNC_SPI);
    gpio_set_function(ice_pmod_3.gpio.io9, GPIO_FUNC_SPI);
    gpio_set_function(ice_pmod_3.gpio.io10, GPIO_FUNC_SPI);
    gpio_set_dir(ice_pmod_3.gpio.io7, GPIO_IN);
    gpio_set_dir(ice_pmod_3.gpio.io8, GPIO_OUT);
    gpio_set_dir(ice_pmod_3.gpio.io9, GPIO_OUT);
    gpio_set_dir(ice_pmod_3.gpio.io10, GPIO_OUT);
    spi_init(spi0, 100000);

    while (1) {
        uint8_t byte;

        tud_task();
        spi_read_blocking(spi0, 0, &byte, 1);

        switch (byte) {
        case 0xFF:
            /* no command */
            break;
        case REMOTE_KEY_VOL_MINUS:
            printf("%s\r\n", "REMOTE_KEY_VOL_MINUS");
            break;
        case REMOTE_KEY_PLAY_PAUSE:
            printf("%s\r\n", "REMOTE_KEY_PLAY_PAUSE");
            break;
        case REMOTE_KEY_VOL_PLUS:
            printf("%s\r\n", "REMOTE_KEY_VOL_PLUS");
            break;
        case REMOTE_KEY_SETUP:
            printf("%s\r\n", "REMOTE_KEY_SETUP");
            break;
        case REMOTE_KEY_UP:
            printf("%s\r\n", "REMOTE_KEY_UP");
            break;
        case REMOTE_KEY_STOP_MODE:
            printf("%s\r\n", "REMOTE_KEY_STOP_MODE");
            break;
        case REMOTE_KEY_LEFT:
            printf("%s\r\n", "REMOTE_KEY_LEFT");
            break;
        case REMOTE_KEY_ENTER:
            printf("%s\r\n", "REMOTE_KEY_ENTER");
            break;
        case REMOTE_KEY_RIGHT:
            printf("%s\r\n", "REMOTE_KEY_RIGHT");
            break;
        case REMOTE_KEY_NUM_0:
            printf("%s\r\n", "REMOTE_KEY_NUM_0");
            break;
        case REMOTE_KEY_BOTTOM:
            printf("%s\r\n", "REMOTE_KEY_BOTTOM");
            break;
        case REMOTE_KEY_RETURN:
            printf("%s\r\n", "REMOTE_KEY_RETURN");
            break;
        case REMOTE_KEY_NUM_1:
            printf("%s\r\n", "REMOTE_KEY_NUM_1");
            break;
        case REMOTE_KEY_NUM_2:
            printf("%s\r\n", "REMOTE_KEY_NUM_2");
            break;
        case REMOTE_KEY_NUM_3:
            printf("%s\r\n", "REMOTE_KEY_NUM_3");
            break;
        case REMOTE_KEY_NUM_4:
            printf("%s\r\n", "REMOTE_KEY_NUM_4");
            break;
        case REMOTE_KEY_NUM_5:
            printf("%s\r\n", "REMOTE_KEY_NUM_5");
            break;
        case REMOTE_KEY_NUM_6:
            printf("%s\r\n", "REMOTE_KEY_NUM_6");
            break;
        case REMOTE_KEY_NUM_7:
            printf("%s\r\n", "REMOTE_KEY_NUM_7");
            break;
        case REMOTE_KEY_NUM_8:
            printf("%s\r\n", "REMOTE_KEY_NUM_8");
            break;
        case REMOTE_KEY_NUM_9:
            printf("%s\r\n", "REMOTE_KEY_NUM_9");
            break;
        default:
            printf("unknown command: 0x%02X\r\n", byte);
        }
    }

    return 0;
}
