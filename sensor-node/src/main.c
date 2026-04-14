#include "pico/stdlib.h"
#include <stdio.h>
#include "hardware/uart.h"

#define LINK_UART_ID uart0
#define LINK_BAUDRATE 115200
#define LINK_UART_TX_PIN 0
#define LINK_UART_RX_PIN 1

int main(void) {
    stdio_init_all();

    sleep_ms(2000);
    printf("sensor-node ready\n");

    uart_init(LINK_UART_ID, LINK_BAUDRATE);
    gpio_set_function(LINK_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(LINK_UART_RX_PIN, GPIO_FUNC_UART);

    // TODO: Confirm board-level UART routing and final pin assignment.
    while (true) {
        uart_puts(LINK_UART_ID, "PING\n");
        printf("sensor-node tx: PING\n");
        sleep_ms(1000);
    }

    return 0;
}
