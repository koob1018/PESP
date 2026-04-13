#include "pico/stdlib.h"

int main(void) {
    stdio_init_all();

    // TODO: Define board-specific UART and GPIO parameters after hardware confirmation.
    while (true) {
        tight_loop_contents();
    }

    return 0;
}
