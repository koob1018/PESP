#include "driver/sensor_node_driver.h"

#include <pico/bootrom.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define SENSOR_EVENT_PIN 15
#define SENSOR_EVENT_CHECK_MS 20
#define NORMAL_POLL_MS 1000
#define NO_REPLY_WARN_MS 2500
#define NO_REPLY_REPEAT_MS 5000

#define FRAME_SOF0 0xA5
#define FRAME_SOF1 0x5A
#define FRAME_MAX_PAYLOAD 160

#define CMD_READ_FORCE 0x10
#define CMD_READ_ALL 0x11
#define CMD_READ_EVENT 0x14
#define CMD_CLEAR_EVENT 0x15
#define CMD_READ_CONFIG 0x16
#define CMD_SET_SAMPLING_RATE 0x20
#define CMD_SET_FORCE_THRESHOLD 0x21
#define CMD_ENABLE_INTERRUPT 0x22
#define CMD_BASE_BOOTSEL 0x7E
#define BASE_BOOTSEL_PAYLOAD "BOOTSEL"

enum frame_rx_state {
    FRAME_WAIT_SOF0,
    FRAME_WAIT_SOF1,
    FRAME_READ_TYPE,
    FRAME_READ_LEN,
    FRAME_READ_PAYLOAD,
    FRAME_READ_CHECKSUM,
};

struct sensor_command {
    uint8_t type;
    const char *payload;
    const char *label;
};

struct sensor_node_driver_api {
    int (*start)(const struct device *dev);
    void (*process)(const struct device *dev);
    int (*request_all)(const struct device *dev);
    int (*request_force)(const struct device *dev);
    int (*request_event)(const struct device *dev);
    int (*clear_event)(const struct device *dev);
    int (*set_force_threshold)(const struct device *dev, uint16_t threshold);
    int (*set_sampling_interval)(const struct device *dev, uint32_t sampling_ms);
    int (*enable_interrupt)(const struct device *dev, bool enabled);
    int (*read_config)(const struct device *dev);
    bool (*get_latest_reading)(const struct device *dev, struct sensor_node_driver_reading *reading);
    bool (*get_latest_event)(const struct device *dev, struct sensor_node_driver_event *event);
    bool (*get_config)(const struct device *dev, struct sensor_node_driver_config *config);
};

struct sensor_node_driver_state {
    const struct device *uart_dev;
    const struct device *event_gpio_dev;
    struct gpio_callback sensor_event_cb;
    volatile bool event_irq_pending;
    bool initialized;
    bool hardware_ready;
    bool logging_enabled;
    sensor_node_driver_log_sink_t log_sink;
    void *log_user_data;

    char rx_buf[FRAME_MAX_PAYLOAD + 1];
    size_t rx_len;
    enum frame_rx_state frame_state;
    uint8_t frame_type;
    uint8_t frame_len;
    uint8_t frame_payload[FRAME_MAX_PAYLOAD + 1];
    uint8_t frame_pos;
    uint8_t frame_checksum;

    uint32_t last_send_ms;
    uint32_t last_rx_ms;
    uint32_t last_warn_ms;
    const char *last_tx_label;
    uint32_t last_event_check_ms;
    bool clear_event_pending;
    bool read_all_next;

    struct sensor_node_driver_reading latest_reading;
    struct sensor_node_driver_event latest_event;
    struct sensor_node_driver_config latest_config;
};

static struct sensor_node_driver_state driver;

static void driver_log(const char *fmt, ...) {
    char buffer[192];
    va_list args;
    int len;

    if (!driver.logging_enabled || driver.log_sink == NULL) {
        return;
    }

    va_start(args, fmt);
    len = vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    if (len < 0) {
        return;
    }
    buffer[sizeof(buffer) - 1] = '\0';
    driver.log_sink(buffer, driver.log_user_data);
}

static void sensor_node_driver_reset_state(void) {
    memset(&driver, 0, sizeof(driver));
    driver.initialized = true;
    driver.frame_state = FRAME_WAIT_SOF0;
    driver.read_all_next = true;
}

static void sensor_node_driver_ensure_initialized(void) {
    if (!driver.initialized) {
        sensor_node_driver_reset_state();
    }
}

void sensor_node_driver_set_logging_enabled(bool enabled) {
    sensor_node_driver_ensure_initialized();
    driver.logging_enabled = enabled;
}

void sensor_node_driver_set_log_sink(sensor_node_driver_log_sink_t sink, void *user_data) {
    sensor_node_driver_ensure_initialized();
    driver.log_sink = sink;
    driver.log_user_data = user_data;
}

static const struct sensor_command read_all_cmd = { CMD_READ_ALL, "", "READ_ALL" };
static const struct sensor_command read_force_cmd = { CMD_READ_FORCE, "", "READ_FORCE" };
static const struct sensor_command read_event_cmd = { CMD_READ_EVENT, "", "READ_EVENT" };
static const struct sensor_command clear_event_cmd = { CMD_CLEAR_EVENT, "", "CLEAR_EVENT" };
static const struct sensor_command read_config_cmd = { CMD_READ_CONFIG, "", "READ_CONFIG" };

static void sensor_event_isr(const struct device *dev, struct gpio_callback *cb, uint32_t pins) {
    ARG_UNUSED(dev);
    ARG_UNUSED(cb);
    ARG_UNUSED(pins);

    driver.event_irq_pending = true;
}

static uint8_t frame_checksum(uint8_t type, uint8_t len, const uint8_t *payload) {
    uint8_t checksum = type ^ len;

    for (uint8_t i = 0; i < len; i++) {
        checksum ^= payload[i];
    }

    return checksum;
}

static bool copy_field_value(const char *payload, const char *key, char *out, size_t out_len) {
    const char *pos = payload;
    size_t key_len = strlen(key);

    if (out == NULL || out_len == 0) {
        return false;
    }

    while (pos != NULL && *pos != '\0') {
        if ((pos == payload || pos[-1] == ',') && strncmp(pos, key, key_len) == 0) {
            const char *value = pos + key_len;
            size_t value_len = 0;

            while (value[value_len] != '\0' && value[value_len] != ',' &&
                   value[value_len] != '\n' && value[value_len] != '\r') {
                value_len++;
            }
            if (value_len >= out_len) {
                value_len = out_len - 1;
            }
            memcpy(out, value, value_len);
            out[value_len] = '\0';
            return true;
        }

        pos = strchr(pos, ',');
        if (pos != NULL) {
            pos++;
        }
    }

    return false;
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

static bool parse_u32_field(const char *payload, const char *key, uint32_t *value) {
    char text[24];

    if (!copy_field_value(payload, key, text, sizeof(text))) {
        return false;
    }

    return parse_u32_text(text, value);
}

static bool parse_bool_field(const char *payload, const char *key, bool *value) {
    uint32_t raw;

    if (value == NULL || !parse_u32_field(payload, key, &raw)) {
        return false;
    }
    if (raw > 1U) {
        return false;
    }

    *value = raw != 0U;
    return true;
}

static bool parse_decimal_scaled_text(const char *text, uint32_t scale, int32_t *value) {
    uint32_t whole = 0;
    uint32_t frac = 0;
    uint32_t frac_scale = 1;
    bool negative = false;
    bool saw_digit = false;
    size_t i = 0;
    int64_t scaled;

    if (text == NULL || value == NULL || scale == 0U) {
        return false;
    }
    if (strcmp(text, "ERR") == 0) {
        return false;
    }
    if (text[i] == '-') {
        negative = true;
        i++;
    }

    while (text[i] >= '0' && text[i] <= '9') {
        uint8_t digit = (uint8_t)(text[i] - '0');

        if (whole > (UINT32_MAX - digit) / 10U) {
            return false;
        }
        whole = whole * 10U + digit;
        saw_digit = true;
        i++;
    }

    if (text[i] == '.') {
        i++;
        while (text[i] >= '0' && text[i] <= '9') {
            if (frac_scale < scale) {
                frac = frac * 10U + (uint8_t)(text[i] - '0');
                frac_scale *= 10U;
            }
            saw_digit = true;
            i++;
        }
    }

    if (!saw_digit || text[i] != '\0') {
        return false;
    }

    while (frac_scale < scale) {
        frac *= 10U;
        frac_scale *= 10U;
    }

    scaled = ((int64_t)whole * (int64_t)scale) + (int64_t)frac;
    if (negative) {
        scaled = -scaled;
    }
    if (scaled > INT32_MAX || scaled < INT32_MIN) {
        return false;
    }

    *value = (int32_t)scaled;
    return true;
}

static bool parse_decimal_scaled_field(const char *payload, const char *key, uint32_t scale, int32_t *value) {
    char text[24];

    if (!copy_field_value(payload, key, text, sizeof(text))) {
        return false;
    }

    return parse_decimal_scaled_text(text, scale, value);
}

static void update_config_from_payload(const char *payload) {
    uint32_t value;
    bool enabled;

    if (parse_u32_field(payload, "SAMPLING_MS=", &value)) {
        driver.latest_config.sampling_ms = value;
        driver.latest_config.valid = true;
    }
    if (parse_u32_field(payload, "FORCE_THRESHOLD=", &value)) {
        driver.latest_config.force_threshold = (uint16_t)value;
        driver.latest_config.valid = true;
    }
    if (parse_bool_field(payload, "INTERRUPT=", &enabled)) {
        driver.latest_config.interrupt_enabled = enabled;
        driver.latest_config.valid = true;
    }
}

static void update_reading_from_payload(const char *payload) {
    struct sensor_node_driver_reading reading = driver.latest_reading;
    int32_t temp;
    int32_t humidity;
    int32_t pressure;
    uint32_t value;
    bool event_latched;
    bool touched = false;

    if (strstr(payload, "TEMP=") != NULL || strstr(payload, "HUM=") != NULL ||
        strstr(payload, "PRESS=") != NULL) {
        bool have_temp = parse_decimal_scaled_field(payload, "TEMP=", 100U, &temp);
        bool have_hum = parse_decimal_scaled_field(payload, "HUM=", 1000U, &humidity);
        bool have_press = parse_decimal_scaled_field(payload, "PRESS=", 100U, &pressure);

        reading.has_environment = have_temp && have_hum && have_press;
        if (reading.has_environment) {
            reading.temperature_centi_c = temp;
            reading.humidity_milli_pct = (uint32_t)humidity;
            reading.pressure_centi_hpa = (uint32_t)pressure;
        }
        touched = true;
    }

    if (copy_field_value(payload, "GAS=", (char[2]){ 0 }, 2)) {
        if (parse_u32_field(payload, "GAS=", &value)) {
            reading.gas_ohms = value;
            reading.has_gas = true;
        } else {
            reading.has_gas = false;
        }
        touched = true;
    }

    if (copy_field_value(payload, "LIGHT=", (char[2]){ 0 }, 2)) {
        if (parse_u32_field(payload, "LIGHT=", &value)) {
            reading.light_raw = (uint16_t)value;
            reading.has_light = true;
        } else {
            reading.has_light = false;
        }
        touched = true;
    }

    if (parse_u32_field(payload, "FORCE=", &value)) {
        reading.force_raw = (uint16_t)value;
        reading.has_force = true;
        touched = true;
    }

    if (parse_bool_field(payload, "EVENT=", &event_latched)) {
        reading.event_latched = event_latched;
        touched = true;
    }

    if (touched) {
        reading.valid = true;
        driver.latest_reading = reading;
    }
}

static void update_event_from_payload(const char *payload) {
    struct sensor_node_driver_event event = driver.latest_event;
    uint32_t value;
    bool latched;

    if (!parse_bool_field(payload, "EVENT=", &latched)) {
        return;
    }

    event.valid = true;
    event.latched = latched;
    if (parse_u32_field(payload, "FORCE=", &value)) {
        event.force_raw = (uint16_t)value;
    }
    if (parse_u32_field(payload, "EVENT_FORCE=", &value)) {
        event.event_force_raw = (uint16_t)value;
    }
    if (parse_u32_field(payload, "AGE_MS=", &value)) {
        event.age_ms = value;
    }
    if (parse_u32_field(payload, "THRESH=", &value)) {
        event.threshold_raw = (uint16_t)value;
    }

    driver.latest_event = event;
}

static int send_frame(const struct sensor_command *cmd) {
    size_t payload_len = strlen(cmd->payload);
    uint8_t checksum;

    if (driver.uart_dev == NULL || !device_is_ready(driver.uart_dev)) {
        return -ENODEV;
    }

    if (payload_len > FRAME_MAX_PAYLOAD) {
        payload_len = FRAME_MAX_PAYLOAD;
    }

    checksum = frame_checksum(cmd->type, (uint8_t)payload_len, (const uint8_t *)cmd->payload);
    uart_poll_out(driver.uart_dev, FRAME_SOF0);
    uart_poll_out(driver.uart_dev, FRAME_SOF1);
    uart_poll_out(driver.uart_dev, cmd->type);
    uart_poll_out(driver.uart_dev, (uint8_t)payload_len);
    for (size_t i = 0; i < payload_len; i++) {
        uart_poll_out(driver.uart_dev, (uint8_t)cmd->payload[i]);
    }
    uart_poll_out(driver.uart_dev, checksum);

    driver.last_send_ms = k_uptime_get_32();
    driver.last_tx_label = cmd->label;
    if (payload_len > 0) {
        driver_log("[tx] %s=%.*s\n", cmd->label, (int)payload_len, cmd->payload);
    } else {
        driver_log("[tx] %s\n", cmd->label);
    }

    return 0;
}

static void handle_payload(const char *payload) {
    driver.last_rx_ms = k_uptime_get_32();

    update_reading_from_payload(payload);
    update_config_from_payload(payload);

    if (strncmp(payload, "FORCE=", 6) == 0) {
        driver_log("[force] %s\n", payload);
    } else if (strncmp(payload, "EVENT=", 6) == 0) {
        update_event_from_payload(payload);
        driver_log("[event] %s\n", payload);
        if (driver.latest_event.latched) {
            driver.clear_event_pending = true;
        }
    } else if (strncmp(payload, "OK,CLEAR_EVENT", 14) == 0) {
        driver.latest_event.latched = false;
        driver.latest_reading.event_latched = false;
        driver_log("[event] cleared\n");
    } else if (strstr(payload, "TEMP=") != NULL) {
        driver_log("[data] %s\n", payload);
        if (driver.latest_reading.event_latched) {
            driver.clear_event_pending = true;
        }
    } else if (strncmp(payload, "OK,", 3) == 0 ||
               strncmp(payload, "SAMPLING_MS=", 12) == 0) {
        driver_log("[config] %s\n", payload);
    } else if (strncmp(payload, "ERR,", 4) == 0) {
        driver_log("[error] %s\n", payload);
    } else if (payload[0] != '\0') {
        driver_log("[rx] %s\n", payload);
    }
}

static bool handle_maintenance_frame(uint8_t type, const uint8_t *payload, uint8_t len) {
    if (type != CMD_BASE_BOOTSEL) {
        return false;
    }
    if (len == strlen(BASE_BOOTSEL_PAYLOAD) &&
        memcmp(payload, BASE_BOOTSEL_PAYLOAD, strlen(BASE_BOOTSEL_PAYLOAD)) == 0) {
        driver_log("[maint] entering BOOTSEL\n");
        k_sleep(K_MSEC(100));
        reset_usb_boot(0, 0);
    }

    driver_log("[maint] ignored frame type=0x%02x\n", type);
    return true;
}

static void maybe_warn_no_reply(uint32_t now) {
    if (driver.last_tx_label == NULL) {
        return;
    }
    if (now - driver.last_rx_ms < NO_REPLY_WARN_MS) {
        return;
    }
    if (now - driver.last_warn_ms < NO_REPLY_REPEAT_MS) {
        return;
    }

    driver.last_warn_ms = now;
    driver_log("[warn] no sensor reply yet; last_tx=%s, check sensor TX -> base RX and common GND\n",
           driver.last_tx_label);
}

static bool feed_frame_byte(uint8_t byte) {
    switch (driver.frame_state) {
    case FRAME_WAIT_SOF0:
        if (byte == FRAME_SOF0) {
            driver.frame_state = FRAME_WAIT_SOF1;
        }
        break;
    case FRAME_WAIT_SOF1:
        driver.frame_state = (byte == FRAME_SOF1) ? FRAME_READ_TYPE : FRAME_WAIT_SOF0;
        break;
    case FRAME_READ_TYPE:
        driver.frame_type = byte;
        driver.frame_checksum = byte;
        driver.frame_state = FRAME_READ_LEN;
        break;
    case FRAME_READ_LEN:
        driver.frame_len = byte;
        driver.frame_pos = 0;
        driver.frame_checksum ^= byte;
        if (driver.frame_len > FRAME_MAX_PAYLOAD) {
            driver.frame_state = FRAME_WAIT_SOF0;
        } else {
            driver.frame_state = (driver.frame_len == 0) ? FRAME_READ_CHECKSUM : FRAME_READ_PAYLOAD;
        }
        break;
    case FRAME_READ_PAYLOAD:
        driver.frame_payload[driver.frame_pos] = byte;
        driver.frame_checksum ^= byte;
        driver.frame_pos++;
        if (driver.frame_pos >= driver.frame_len) {
            driver.frame_state = FRAME_READ_CHECKSUM;
        }
        break;
    case FRAME_READ_CHECKSUM:
        driver.frame_state = FRAME_WAIT_SOF0;
        return byte == driver.frame_checksum;
    }

    return false;
}

static void process_uart_rx(void) {
    uint8_t ch = 0;

    if (!driver.hardware_ready) {
        return;
    }

    while (uart_poll_in(driver.uart_dev, &ch) == 0) {
        if (driver.frame_state != FRAME_WAIT_SOF0 || ch == FRAME_SOF0) {
            if (feed_frame_byte(ch)) {
                driver.frame_payload[driver.frame_len] = '\0';
                if (!handle_maintenance_frame(driver.frame_type, driver.frame_payload,
                                              driver.frame_len)) {
                    handle_payload((const char *)driver.frame_payload);
                }
            }
            continue;
        }

        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            driver.rx_buf[driver.rx_len] = '\0';
            handle_payload(driver.rx_buf);
            driver.rx_len = 0;
        } else if (driver.rx_len < (sizeof(driver.rx_buf) - 1)) {
            driver.rx_buf[driver.rx_len++] = (char)ch;
        } else {
            driver.rx_len = 0;
        }
    }
}

static int sensor_node_driver_device_init(const struct device *dev) {
    ARG_UNUSED(dev);

    sensor_node_driver_reset_state();

    return 0;
}

static int configure_hardware(void) {
    int ret;

    if (driver.hardware_ready) {
        return 0;
    }

    driver.uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));
    driver.event_gpio_dev = DEVICE_DT_GET(DT_NODELABEL(gpio0));

    if (!device_is_ready(driver.uart_dev)) {
        driver_log("[error] uart0 not ready\n");
        return -ENODEV;
    }
    if (!device_is_ready(driver.event_gpio_dev)) {
        driver_log("[error] gpio0 not ready\n");
        return -ENODEV;
    }

    ret = gpio_pin_configure(driver.event_gpio_dev, SENSOR_EVENT_PIN, GPIO_INPUT | GPIO_PULL_DOWN);
    if (ret != 0) {
        driver_log("[error] gpio%u configure failed ret=%d\n", SENSOR_EVENT_PIN, ret);
        return ret;
    }

    gpio_init_callback(&driver.sensor_event_cb, sensor_event_isr, BIT(SENSOR_EVENT_PIN));
    ret = gpio_add_callback(driver.event_gpio_dev, &driver.sensor_event_cb);
    if (ret != 0) {
        driver_log("[error] gpio callback failed ret=%d\n", ret);
        return ret;
    }

    ret = gpio_pin_interrupt_configure(driver.event_gpio_dev, SENSOR_EVENT_PIN, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0) {
        driver_log("[error] gpio%u irq configure failed ret=%d\n", SENSOR_EVENT_PIN, ret);
        return ret;
    }

    driver.hardware_ready = true;
    return 0;
}

static int sensor_node_driver_start_impl(const struct device *dev) {
    int ret;

    ARG_UNUSED(dev);

    sensor_node_driver_ensure_initialized();

    ret = configure_hardware();
    if (ret != 0) {
        return ret;
    }

    driver.last_send_ms = k_uptime_get_32();
    driver.last_rx_ms = driver.last_send_ms;
    driver.last_warn_ms = driver.last_send_ms;
    driver.last_event_check_ms = driver.last_send_ms;
    driver_log("[driver] sensor-node ready irq_gpio=%u\n", SENSOR_EVENT_PIN);
    return 0;
}

static void sensor_node_driver_process_impl(const struct device *dev) {
    uint32_t now = k_uptime_get_32();

    ARG_UNUSED(dev);

    if (!driver.hardware_ready) {
        return;
    }

    process_uart_rx();

    if (now - driver.last_event_check_ms >= SENSOR_EVENT_CHECK_MS) {
        driver.last_event_check_ms = now;
        if (gpio_pin_get(driver.event_gpio_dev, SENSOR_EVENT_PIN) > 0) {
            driver.event_irq_pending = true;
        }
    }

    if (driver.clear_event_pending) {
        (void)send_frame(&clear_event_cmd);
        driver.clear_event_pending = false;
    } else if (driver.event_irq_pending) {
        (void)send_frame(&read_event_cmd);
        driver.event_irq_pending = false;
    } else if (now - driver.last_send_ms >= NORMAL_POLL_MS) {
        (void)send_frame(driver.read_all_next ? &read_all_cmd : &read_force_cmd);
        driver.read_all_next = !driver.read_all_next;
    }

    maybe_warn_no_reply(now);
}

static int sensor_node_driver_request_all_impl(const struct device *dev) {
    ARG_UNUSED(dev);

    return send_frame(&read_all_cmd);
}

static int sensor_node_driver_request_force_impl(const struct device *dev) {
    ARG_UNUSED(dev);

    return send_frame(&read_force_cmd);
}

static int sensor_node_driver_request_event_impl(const struct device *dev) {
    ARG_UNUSED(dev);

    return send_frame(&read_event_cmd);
}

static int sensor_node_driver_clear_event_impl(const struct device *dev) {
    ARG_UNUSED(dev);

    return send_frame(&clear_event_cmd);
}

static int sensor_node_driver_set_force_threshold_impl(const struct device *dev, uint16_t threshold) {
    char payload[8];
    struct sensor_command cmd = {
        .type = CMD_SET_FORCE_THRESHOLD,
        .payload = payload,
        .label = "SET_FORCE_THRESHOLD",
    };

    ARG_UNUSED(dev);

    snprintf(payload, sizeof(payload), "%u", threshold);
    return send_frame(&cmd);
}

static int sensor_node_driver_set_sampling_interval_impl(const struct device *dev, uint32_t sampling_ms) {
    char payload[12];
    struct sensor_command cmd = {
        .type = CMD_SET_SAMPLING_RATE,
        .payload = payload,
        .label = "SET_SAMPLING_RATE",
    };

    ARG_UNUSED(dev);

    snprintf(payload, sizeof(payload), "%lu", (unsigned long)sampling_ms);
    return send_frame(&cmd);
}

static int sensor_node_driver_enable_interrupt_impl(const struct device *dev, bool enabled) {
    struct sensor_command cmd = {
        .type = CMD_ENABLE_INTERRUPT,
        .payload = enabled ? "1" : "0",
        .label = "ENABLE_INTERRUPT",
    };

    ARG_UNUSED(dev);

    return send_frame(&cmd);
}

static int sensor_node_driver_read_config_impl(const struct device *dev) {
    ARG_UNUSED(dev);

    return send_frame(&read_config_cmd);
}

static bool sensor_node_driver_get_latest_reading_impl(const struct device *dev,
                                                       struct sensor_node_driver_reading *reading) {
    ARG_UNUSED(dev);

    if (reading == NULL || !driver.latest_reading.valid) {
        return false;
    }

    *reading = driver.latest_reading;
    return true;
}

static bool sensor_node_driver_get_latest_event_impl(const struct device *dev,
                                                     struct sensor_node_driver_event *event) {
    ARG_UNUSED(dev);

    if (event == NULL || !driver.latest_event.valid) {
        return false;
    }

    *event = driver.latest_event;
    return true;
}

static bool sensor_node_driver_get_config_impl(const struct device *dev,
                                               struct sensor_node_driver_config *config) {
    ARG_UNUSED(dev);

    if (config == NULL || !driver.latest_config.valid) {
        return false;
    }

    *config = driver.latest_config;
    return true;
}

int sensor_node_driver_start(const struct device *dev) {
    return sensor_node_driver_start_impl(dev);
}

void sensor_node_driver_process(const struct device *dev) {
    sensor_node_driver_process_impl(dev);
}

int sensor_node_driver_request_all(const struct device *dev) {
    return sensor_node_driver_request_all_impl(dev);
}

int sensor_node_driver_request_force(const struct device *dev) {
    return sensor_node_driver_request_force_impl(dev);
}

int sensor_node_driver_request_event(const struct device *dev) {
    return sensor_node_driver_request_event_impl(dev);
}

int sensor_node_driver_clear_event(const struct device *dev) {
    return sensor_node_driver_clear_event_impl(dev);
}

int sensor_node_driver_set_force_threshold(const struct device *dev, uint16_t threshold) {
    return sensor_node_driver_set_force_threshold_impl(dev, threshold);
}

int sensor_node_driver_set_sampling_interval(const struct device *dev, uint32_t sampling_ms) {
    return sensor_node_driver_set_sampling_interval_impl(dev, sampling_ms);
}

int sensor_node_driver_enable_interrupt(const struct device *dev, bool enabled) {
    return sensor_node_driver_enable_interrupt_impl(dev, enabled);
}

int sensor_node_driver_read_config(const struct device *dev) {
    return sensor_node_driver_read_config_impl(dev);
}

bool sensor_node_driver_get_latest_reading(const struct device *dev,
                                           struct sensor_node_driver_reading *reading) {
    return sensor_node_driver_get_latest_reading_impl(dev, reading);
}

bool sensor_node_driver_get_latest_event(const struct device *dev,
                                         struct sensor_node_driver_event *event) {
    return sensor_node_driver_get_latest_event_impl(dev, event);
}

bool sensor_node_driver_get_config(const struct device *dev, struct sensor_node_driver_config *config) {
    return sensor_node_driver_get_config_impl(dev, config);
}

static const struct sensor_node_driver_api sensor_node_api = {
    .start = sensor_node_driver_start_impl,
    .process = sensor_node_driver_process_impl,
    .request_all = sensor_node_driver_request_all_impl,
    .request_force = sensor_node_driver_request_force_impl,
    .request_event = sensor_node_driver_request_event_impl,
    .clear_event = sensor_node_driver_clear_event_impl,
    .set_force_threshold = sensor_node_driver_set_force_threshold_impl,
    .set_sampling_interval = sensor_node_driver_set_sampling_interval_impl,
    .enable_interrupt = sensor_node_driver_enable_interrupt_impl,
    .read_config = sensor_node_driver_read_config_impl,
    .get_latest_reading = sensor_node_driver_get_latest_reading_impl,
    .get_latest_event = sensor_node_driver_get_latest_event_impl,
    .get_config = sensor_node_driver_get_config_impl,
};

DEVICE_DEFINE(sensor_node_driver, "sensor_node_driver",
              sensor_node_driver_device_init, NULL, NULL, NULL,
              POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT,
              &sensor_node_api);
