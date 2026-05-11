#include "driver/sensor_node_driver.h"

#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
#include <pico/bootrom.h>
#endif

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define CONFIG_GAP_MS 250
#define MAINT_CMD_MAX_LEN 32
#define DRIVER_RETRY_MS 1000

#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
static const struct device *const console_dev = DEVICE_DT_GET(DT_NODELABEL(cdc_acm_uart0));
#endif

#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
static volatile bool usb_console_enabled;
static volatile bool console_output_enabled;
#endif

static void console_write(const char *message) {
#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
    if (!usb_console_enabled || message == NULL) {
        return;
    }

    for (size_t i = 0; message[i] != '\0'; i++) {
        uart_poll_out(console_dev, (unsigned char)message[i]);
    }
#else
    ARG_UNUSED(message);
#endif
}

static void app_log(const char *fmt, ...) {
#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
    char buffer[192];
    va_list args;
    int len;

    if (!console_output_enabled) {
        return;
    }

    va_start(args, fmt);
    len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (len < 0) {
        return;
    }
    buffer[sizeof(buffer) - 1] = '\0';
    console_write(buffer);
#else
    ARG_UNUSED(fmt);
#endif
}

static void driver_log_sink(const char *message, void *user_data) {
    ARG_UNUSED(user_data);
    console_write(message);
}

#define APP_LOG(...) app_log(__VA_ARGS__)

#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
static void enter_bootsel(void) {
    k_sleep(K_MSEC(100));
    reset_usb_boot(0, 0);
}

static void enable_console_output(void) {
    console_output_enabled = true;
    sensor_node_driver_set_logging_enabled(true);
}

static void handle_maintenance_command(const char *cmd) {
    enable_console_output();

    if (strcmp(cmd, "BOOTSEL") == 0 || strcmp(cmd, "REBOOT_BOOTSEL") == 0) {
        enter_bootsel();
    } else if (strcmp(cmd, "PING") == 0) {
        APP_LOG("[maint] PONG\n");
    } else if (cmd[0] != '\0') {
        APP_LOG("[maint] unknown command: %s\n", cmd);
    }
}

static void process_maintenance_console(void) {
    static char cmd[MAINT_CMD_MAX_LEN];
    static size_t cmd_len;
    uint8_t ch;

    if (!usb_console_enabled) {
        return;
    }

    while (uart_poll_in(console_dev, &ch) == 0) {
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            cmd[cmd_len] = '\0';
            handle_maintenance_command(cmd);
            cmd_len = 0;
        } else if (cmd_len < (sizeof(cmd) - 1)) {
            cmd[cmd_len++] = (char)ch;
        } else {
            cmd_len = 0;
        }
    }
}

static void maintenance_thread(void *unused0, void *unused1, void *unused2) {
    ARG_UNUSED(unused0);
    ARG_UNUSED(unused1);
    ARG_UNUSED(unused2);

    while (1) {
        process_maintenance_console();
        k_sleep(K_MSEC(10));
    }
}

K_THREAD_DEFINE(maintenance_thread_id, 1024, maintenance_thread, NULL, NULL, NULL, 7, 0, 0);
#endif

static void service_for(const struct device *sensor_node, uint32_t duration_ms) {
    uint32_t start_ms = k_uptime_get_32();

    while (k_uptime_get_32() - start_ms < duration_ms) {
        sensor_node_driver_process(sensor_node);
        k_sleep(K_MSEC(1));
    }
}

static int configure_sensor_node(const struct device *sensor_node) {
    int ret;

    ret = sensor_node_driver_set_force_threshold(sensor_node, 1000);
    if (ret != 0) {
        return ret;
    }
    service_for(sensor_node, CONFIG_GAP_MS);

    ret = sensor_node_driver_set_sampling_interval(sensor_node, 2000);
    if (ret != 0) {
        return ret;
    }
    service_for(sensor_node, CONFIG_GAP_MS);

    ret = sensor_node_driver_enable_interrupt(sensor_node, true);
    if (ret != 0) {
        return ret;
    }
    service_for(sensor_node, CONFIG_GAP_MS);

    ret = sensor_node_driver_read_config(sensor_node);
    if (ret != 0) {
        return ret;
    }
    service_for(sensor_node, CONFIG_GAP_MS);

    return 0;
}

int main(void) {
    const struct device *sensor_node = DEVICE_GET(sensor_node_driver);
    uint32_t last_retry_ms = 0;
    bool driver_started = false;
    int ret;

#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
    usb_console_enabled = device_is_ready(console_dev);
#endif
    sensor_node_driver_set_log_sink(driver_log_sink, NULL);
    k_sleep(K_MSEC(1000));
    sensor_node_driver_set_logging_enabled(false);
    APP_LOG("[app] base-station driver demo start\n");
    APP_LOG("[maint] send BOOTSEL over this USB serial port to enter ROM bootloader\n");

    if (!device_is_ready(sensor_node)) {
        APP_LOG("[warn] sensor-node driver device not ready yet; will retry\n");
    }

    ret = sensor_node_driver_start(sensor_node);
    if (ret != 0) {
        APP_LOG("[error] sensor-node driver start failed ret=%d\n", ret);
    } else {
        ret = configure_sensor_node(sensor_node);
        if (ret != 0) {
            APP_LOG("[error] sensor-node configuration failed ret=%d\n", ret);
        } else {
            driver_started = true;
        }
    }

    while (1) {
        if (driver_started) {
            sensor_node_driver_process(sensor_node);
        } else if (k_uptime_get_32() - last_retry_ms >= DRIVER_RETRY_MS) {
            last_retry_ms = k_uptime_get_32();
            ret = sensor_node_driver_start(sensor_node);
            if (ret == 0) {
                ret = configure_sensor_node(sensor_node);
                driver_started = ret == 0;
                if (!driver_started) {
                    APP_LOG("[error] sensor-node configuration failed ret=%d\n", ret);
                }
            } else {
                APP_LOG("[error] sensor-node driver retry failed ret=%d\n", ret);
            }
        }
        k_sleep(K_MSEC(1));
    }

    return 0;
}
