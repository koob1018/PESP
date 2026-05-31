#include "driver/sensor_node_driver.h"

#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
#include <pico/bootrom.h>
#endif

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define CONFIG_GAP_MS 250
#define HOST_CMD_MAX_LEN 80
#define DRIVER_RETRY_MS 1000
#define DEFAULT_FORCE_THRESHOLD 1000
#define DEFAULT_SAMPLING_MS 2000
#define DEFAULT_LIGHT_HIGH_THRESHOLD 12000
#define DEFAULT_TEMP_HIGH "30.00"
#define DEFAULT_HUMIDITY_HIGH "70.00"

#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
static const struct device *const console_dev = DEVICE_DT_GET(DT_NODELABEL(cdc_acm_uart0));
#endif

#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
static volatile bool usb_console_enabled;
static volatile bool console_output_enabled;
K_MSGQ_DEFINE(host_rx_msgq, sizeof(uint8_t), 256, 4);
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

static bool parse_u32_text(const char *text, uint32_t *value) {
    uint32_t parsed = 0;
    bool saw_digit = false;

    if (text == NULL || value == NULL || text[0] == '\0') {
        return false;
    }

    for (size_t i = 0; text[i] != '\0'; i++) {
        uint8_t digit;

        if (text[i] < '0' || text[i] > '9') {
            return false;
        }
        digit = (uint8_t)(text[i] - '0');
        if (parsed > (UINT32_MAX - digit) / 10U) {
            return false;
        }
        parsed = parsed * 10U + digit;
        saw_digit = true;
    }

    if (!saw_digit) {
        return false;
    }

    *value = parsed;
    return true;
}

static bool split_prefixed_command(const char *cmd, const char *prefix, const char **value) {
    size_t prefix_len = strlen(prefix);

    if (strncmp(cmd, prefix, prefix_len) != 0 || cmd[prefix_len] == '\0') {
        return false;
    }

    *value = cmd + prefix_len;
    return true;
}

static void host_console_uart_isr(const struct device *dev, void *user_data) {
    uint8_t ch;

    ARG_UNUSED(user_data);

    while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
        if (!uart_irq_rx_ready(dev)) {
            continue;
        }

        while (uart_fifo_read(dev, &ch, 1) == 1) {
            (void)k_msgq_put(&host_rx_msgq, &ch, K_NO_WAIT);
        }
    }
}

static void init_host_console_rx(void) {
    int ret;

    if (!usb_console_enabled) {
        return;
    }

    ret = uart_irq_callback_user_data_set(console_dev, host_console_uart_isr, NULL);
    if (ret != 0) {
        APP_LOG("[warn] USB CDC RX callback failed ret=%d\n", ret);
        return;
    }

    uart_irq_rx_enable(console_dev);
}

static void handle_host_command(const struct device *sensor_node, const char *cmd) {
    const char *value_text = NULL;
    uint32_t value;

    enable_console_output();
    APP_LOG("[host-rx] %s\n", cmd);

    if (strcmp(cmd, "BOOTSEL") == 0 || strcmp(cmd, "REBOOT_BOOTSEL") == 0) {
        enter_bootsel();
    } else if (strcmp(cmd, "PING") == 0) {
        APP_LOG("[maint] PONG\n");
    } else if (strcmp(cmd, "READ_ALL") == 0) {
        (void)sensor_node_driver_request_all(sensor_node);
    } else if (strcmp(cmd, "READ_FORCE") == 0) {
        (void)sensor_node_driver_request_force(sensor_node);
    } else if (strcmp(cmd, "READ_EVENT") == 0) {
        (void)sensor_node_driver_request_event(sensor_node);
    } else if (strcmp(cmd, "CLEAR_EVENT") == 0) {
        (void)sensor_node_driver_clear_event(sensor_node);
    } else if (strcmp(cmd, "READ_CONFIG") == 0) {
        (void)sensor_node_driver_read_config(sensor_node);
    } else if (split_prefixed_command(cmd, "SET_FORCE_THRESHOLD=", &value_text) &&
               parse_u32_text(value_text, &value)) {
        if (value > UINT16_MAX) {
            value = UINT16_MAX;
        }
        (void)sensor_node_driver_set_force_threshold(sensor_node, (uint16_t)value);
    } else if (split_prefixed_command(cmd, "SET_SAMPLING_RATE=", &value_text) &&
               parse_u32_text(value_text, &value)) {
        (void)sensor_node_driver_set_sampling_interval(sensor_node, value);
    } else if (split_prefixed_command(cmd, "ENABLE_INTERRUPT=", &value_text) &&
               parse_u32_text(value_text, &value)) {
        (void)sensor_node_driver_enable_interrupt(sensor_node, value != 0U);
    } else if (split_prefixed_command(cmd, "SET_LIGHT_HIGH=", &value_text) &&
               parse_u32_text(value_text, &value)) {
        if (value > UINT16_MAX) {
            value = UINT16_MAX;
        }
        (void)sensor_node_driver_set_light_high(sensor_node, (uint16_t)value);
    } else if (split_prefixed_command(cmd, "SET_TEMP_HIGH=", &value_text)) {
        (void)sensor_node_driver_set_temp_high(sensor_node, value_text);
    } else if (split_prefixed_command(cmd, "SET_HUMIDITY_HIGH=", &value_text)) {
        (void)sensor_node_driver_set_humidity_high(sensor_node, value_text);
    } else if (split_prefixed_command(cmd, "SERVICE_MODE=", &value_text) &&
               parse_u32_text(value_text, &value)) {
        (void)sensor_node_driver_set_service_mode(sensor_node, value != 0U);
    } else if (cmd[0] != '\0') {
        APP_LOG("[maint] unknown host command: %s\n", cmd);
    }
}

static void process_host_console(const struct device *sensor_node) {
    static char cmd[HOST_CMD_MAX_LEN];
    static size_t cmd_len;
    uint8_t ch;

    if (!usb_console_enabled) {
        return;
    }

    while (k_msgq_get(&host_rx_msgq, &ch, K_NO_WAIT) == 0) {
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            cmd[cmd_len] = '\0';
            handle_host_command(sensor_node, cmd);
            cmd_len = 0;
        } else if (cmd_len < (sizeof(cmd) - 1)) {
            cmd[cmd_len++] = (char)ch;
        } else {
            cmd_len = 0;
        }
    }
}
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

    ret = sensor_node_driver_set_force_threshold(sensor_node, DEFAULT_FORCE_THRESHOLD);
    if (ret != 0) {
        return ret;
    }
    service_for(sensor_node, CONFIG_GAP_MS);

    ret = sensor_node_driver_set_sampling_interval(sensor_node, DEFAULT_SAMPLING_MS);
    if (ret != 0) {
        return ret;
    }
    service_for(sensor_node, CONFIG_GAP_MS);

    ret = sensor_node_driver_set_light_high(sensor_node, DEFAULT_LIGHT_HIGH_THRESHOLD);
    if (ret != 0) {
        return ret;
    }
    service_for(sensor_node, CONFIG_GAP_MS);

    ret = sensor_node_driver_set_temp_high(sensor_node, DEFAULT_TEMP_HIGH);
    if (ret != 0) {
        return ret;
    }
    service_for(sensor_node, CONFIG_GAP_MS);

    ret = sensor_node_driver_set_humidity_high(sensor_node, DEFAULT_HUMIDITY_HIGH);
    if (ret != 0) {
        return ret;
    }
    service_for(sensor_node, CONFIG_GAP_MS);

    ret = sensor_node_driver_set_service_mode(sensor_node, false);
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
    console_output_enabled = usb_console_enabled;
    init_host_console_rx();
#endif
    sensor_node_driver_set_log_sink(driver_log_sink, NULL);
    k_sleep(K_MSEC(1000));
    sensor_node_driver_set_logging_enabled(true);
    APP_LOG("[app] base-station driver demo start\n");
    APP_LOG("[maint] GUI serial endpoint ready; send BOOTSEL to enter ROM bootloader\n");

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
#if defined(CONFIG_USB_DEVICE_STACK_NEXT)
        process_host_console(sensor_node);
#endif

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
