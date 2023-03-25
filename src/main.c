// pico-sdk
#include "pico/stdlib.h"
#include "pico/stdlib.h"
#include "pico/stdio.h"
#include "boards/pico_ice.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"

// micro-ros
#include "pico_uart_transports.h"
#include <rcl/rcl.h>
#include <rcl/error_handling.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <std_msgs/msg/int32.h>
#include <rmw_microros/rmw_microros.h>

// pico-ice
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
#define REMOTE_KEY_DOWN         0xB0
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

#define LED_PIN 25

#define RCCHECK(x) if ((x) != RCL_RET_OK) __asm__("bkpt")

rclc_executor_t ros_executor;
rcl_publisher_t ros_publisher;
std_msgs__msg__Int32 ros_msg;

void board_ice_init(void) {
    tusb_init();
    stdio_usb_init();

    // Let the FPGA start and give it a clock
    ice_fpga_init(48);
    ice_fpga_start();
}

void board_gpio_init(void) {
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);
}

void board_spi_init(void) {
    gpio_init(ice_pmod_3.gpio.io1);
    gpio_init(ice_pmod_3.gpio.io2);
    gpio_init(ice_pmod_3.gpio.io3);
    gpio_init(ice_pmod_3.gpio.io4);

    gpio_set_function(ice_pmod_3.gpio.io7, GPIO_FUNC_SPI);
    gpio_set_function(ice_pmod_3.gpio.io8, GPIO_FUNC_SPI);
    gpio_set_function(ice_pmod_3.gpio.io9, GPIO_FUNC_SPI);
    gpio_set_function(ice_pmod_3.gpio.io10, GPIO_FUNC_SPI);
    gpio_set_dir(ice_pmod_3.gpio.io7, GPIO_IN);
    gpio_set_dir(ice_pmod_3.gpio.io8, GPIO_OUT);
    gpio_set_dir(ice_pmod_3.gpio.io9, GPIO_OUT);
    gpio_set_dir(ice_pmod_3.gpio.io10, GPIO_OUT);
    spi_init(spi0, 100000);
}

void timer_callback(rcl_timer_t *timer, int64_t last_call_time)
{
    uint8_t byte;

    spi_read_blocking(spi0, 0, &byte, 1);

    switch (byte) {
    case 0xFF:
        /* no command */
        break;
    case REMOTE_KEY_VOL_MINUS:
    case REMOTE_KEY_PLAY_PAUSE:
    case REMOTE_KEY_VOL_PLUS:
    case REMOTE_KEY_SETUP:
    case REMOTE_KEY_STOP_MODE:
    case REMOTE_KEY_LEFT:
    case REMOTE_KEY_RIGHT:
    case REMOTE_KEY_UP:
    case REMOTE_KEY_DOWN:
    case REMOTE_KEY_ENTER:
    case REMOTE_KEY_RETURN:
    case REMOTE_KEY_NUM_0:
    case REMOTE_KEY_NUM_1:
    case REMOTE_KEY_NUM_2:
    case REMOTE_KEY_NUM_3:
    case REMOTE_KEY_NUM_4:
    case REMOTE_KEY_NUM_5:
    case REMOTE_KEY_NUM_6:
    case REMOTE_KEY_NUM_7:
    case REMOTE_KEY_NUM_8:
    case REMOTE_KEY_NUM_9:
        break;
    }

    RCCHECK(rcl_publish(&ros_publisher, &ros_msg, NULL));
}

int main(void)
{
    rcl_timer_t timer = {0};
    rcl_node_t node = {0};
    rcl_allocator_t allocator = {0};
    rclc_support_t support = {0};

    // custom initialization
    board_gpio_init();
    board_spi_init();
    board_ice_init();

    rmw_uros_set_custom_transport(
        true,
        NULL,
        pico_serial_transport_open,
        pico_serial_transport_close,
        pico_serial_transport_write,
        pico_serial_transport_read
    );

    allocator = rcl_get_default_allocator();
    //RCCHECK(rmw_uros_ping_agent(1000 /*ms*/, 120 /*attempts*/));
    while(1) {
        tud_task();
    }
    RCCHECK(rclc_support_init(&support, 0, NULL, &allocator));
    RCCHECK(rclc_node_init_default(&node, "pico_node", "", &support));
    RCCHECK(rclc_publisher_init_default(&ros_publisher, &node, ROSIDL_GET_MSG_TYPE_SUPPORT(std_msgs, msg, Int32), "pico_publisher"));
    RCCHECK(rclc_executor_init(&ros_executor, &support.context, 1, &allocator));
    RCCHECK(rclc_timer_init_default(&timer, &support, RCL_MS_TO_NS(1000), &timer_callback));
    RCCHECK(rclc_executor_add_timer(&ros_executor, &timer));

    gpio_put(LED_PIN, 1);

    while (true) {
        tud_task();
        rclc_executor_spin_some(&ros_executor, RCL_MS_TO_NS(100));
    }
    return 0;
}
