#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/printk.h>

int main(void) {
    const struct device *const console_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
    const struct device *const link_uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));
    uint32_t dtr = 0;
    char rx_buf[32];
    size_t rx_len = 0;

    if (!device_is_ready(link_uart_dev)) {
        printk("base-station uart0 not ready\n");
        return -1;
    }

    while (!dtr) {
        uart_line_ctrl_get(console_dev, UART_LINE_CTRL_DTR, &dtr);
        k_sleep(K_MSEC(100));
    }

    printk("base-station skeleton ready\n");

    // TODO: Confirm board-level UART wiring and final pin assignment.
    while (1) {
        uint8_t ch = 0;

        if (uart_poll_in(link_uart_dev, &ch) == 0) {
            if (ch == '\n') {
                rx_buf[rx_len] = '\0';
                printk("base-station rx: %s\n", rx_buf);
                rx_len = 0;
            } else if (rx_len < (sizeof(rx_buf) - 1)) {
                rx_buf[rx_len++] = (char)ch;
            } else {
                rx_len = 0;
            }
        }

        k_sleep(K_MSEC(10));
    }

    return 0;
}
