#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

int main(void) {
    printk("base-station skeleton ready\n");

    // TODO: Confirm UART instance and interrupt GPIO assignment with hardware design.
    while (1) {
        k_sleep(K_SECONDS(1));
    }

    return 0;
}
