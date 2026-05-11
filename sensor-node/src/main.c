#include "pico/stdlib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/uart.h"

#include "bme680_min.h"
#include "veml7700_min.h"

#define LINK_UART_ID uart0
#define LINK_BAUDRATE 115200
#define LINK_UART_TX_PIN 0
#define LINK_UART_RX_PIN 1

#define FSR_ADC_GPIO 26
#define FSR_ADC_INPUT 0

#define SENSOR_I2C i2c1
#define SENSOR_I2C_SDA_PIN 6
#define SENSOR_I2C_SCL_PIN 7
#define VEML7700_ADDR 0x10
#define BME680_CHIP_ID_REG 0xD0

#define EVENT_IRQ_PIN 15
#define DEFAULT_SAMPLING_MS 2000
#define MIN_SAMPLING_MS 100
#define DEFAULT_FORCE_THRESHOLD 1000
#define FORCE_HYSTERESIS 50
#define FORCE_EVENT_SAMPLE_MS 20

#define FRAME_SOF0 0xA5
#define FRAME_SOF1 0x5A
#define FRAME_RESPONSE_FLAG 0x80
#define FRAME_MAX_PAYLOAD 160
#define USB_CMD_MAX_LEN 48

#define CMD_PING 0x01
#define CMD_READ_FORCE 0x10
#define CMD_READ_ALL 0x11
#define CMD_READ_ENVIRONMENT 0x12
#define CMD_READ_LIGHT 0x13
#define CMD_READ_EVENT 0x14
#define CMD_CLEAR_EVENT 0x15
#define CMD_READ_CONFIG 0x16
#define CMD_SET_SAMPLING_RATE 0x20
#define CMD_SET_FORCE_THRESHOLD 0x21
#define CMD_ENABLE_INTERRUPT 0x22
#define CMD_BASE_BOOTSEL 0x7E

enum frame_rx_state {
    FRAME_WAIT_SOF0,
    FRAME_WAIT_SOF1,
    FRAME_READ_TYPE,
    FRAME_READ_LEN,
    FRAME_READ_PAYLOAD,
    FRAME_READ_CHECKSUM,
};

static uint32_t sampling_interval_ms = DEFAULT_SAMPLING_MS;
static uint16_t force_threshold = DEFAULT_FORCE_THRESHOLD;
static bool interrupt_enabled = true;
static bool event_latched;
static bool force_was_high;
static uint16_t latest_force;
static uint16_t event_force;
static uint32_t event_time_ms;
static bool reply_as_frame;
static uint8_t reply_frame_type;

static uint16_t read_force_raw(void) {
    adc_select_input(FSR_ADC_INPUT);
    return adc_read();
}

static void update_event_irq_pin(void) {
    gpio_put(EVENT_IRQ_PIN, interrupt_enabled && event_latched);
}

static void update_force_event(uint16_t force_raw) {
    latest_force = force_raw;

    if (!force_was_high && force_raw >= force_threshold) {
        event_latched = true;
        event_force = force_raw;
        event_time_ms = to_ms_since_boot(get_absolute_time());
        force_was_high = true;
        update_event_irq_pin();
        printf("[event] latched force=%u threshold=%u\n", (unsigned)event_force, (unsigned)force_threshold);
    } else if (force_was_high && (uint32_t)force_raw + FORCE_HYSTERESIS < force_threshold) {
        force_was_high = false;
    }
}

static bool parse_u32_arg(const char *text, const char *prefix, uint32_t *value) {
    size_t prefix_len = strlen(prefix);
    char *end = NULL;
    unsigned long parsed;

    if (strncmp(text, prefix, prefix_len) != 0 || text[prefix_len] == '\0') {
        return false;
    }

    parsed = strtoul(text + prefix_len, &end, 10);
    if (end == text + prefix_len || *end != '\0') {
        return false;
    }

    *value = (uint32_t)parsed;
    return true;
}

static bool read_environment(float *temp, float *hum, float *pres, uint32_t *gas_ohms, bool *gas_valid) {
    if (gas_ohms != NULL) {
        *gas_ohms = 0;
    }
    if (gas_valid != NULL) {
        *gas_valid = false;
    }

    for (int addr = 0x76; addr <= 0x77; addr++) {
        if (bme680_init_min(SENSOR_I2C, addr) &&
            bme680_read_all_min(SENSOR_I2C, addr, temp, hum, pres, gas_ohms, gas_valid)) {
            return true;
        }
    }

    return false;
}

static bool read_light(uint16_t *light_raw) {
    return veml7700_init_min(SENSOR_I2C, VEML7700_ADDR) &&
           veml7700_read_raw(SENSOR_I2C, VEML7700_ADDR, light_raw);
}

static bool i2c_probe_addr(uint8_t addr) {
    uint8_t value = 0;

    return i2c_read_blocking(SENSOR_I2C, addr, &value, 1, false) >= 0;
}

static bool bme680_read_chip_id(uint8_t addr, uint8_t *chip_id) {
    uint8_t reg = BME680_CHIP_ID_REG;

    if (chip_id == NULL) {
        return false;
    }
    if (i2c_write_blocking(SENSOR_I2C, addr, &reg, 1, true) < 0) {
        return false;
    }
    return i2c_read_blocking(SENSOR_I2C, addr, chip_id, 1, false) >= 0;
}

static void print_i2c_scan(void) {
    bool found = false;

    printf("[i2c] scan start\n");
    for (uint8_t addr = 0x08; addr <= 0x77; addr++) {
        if (i2c_probe_addr(addr)) {
            printf("[i2c] addr=0x%02x\n", addr);
            found = true;
        }
    }
    if (!found) {
        printf("[i2c] no devices found\n");
    }
    printf("[i2c] scan done\n");
}

static void print_bme_probe(void) {
    for (uint8_t addr = 0x76; addr <= 0x77; addr++) {
        uint8_t chip_id = 0;

        if (bme680_read_chip_id(addr, &chip_id)) {
            printf("[bme] addr=0x%02x chip_id=0x%02x\n", addr, chip_id);
        } else {
            printf("[bme] addr=0x%02x no-reply\n", addr);
        }
    }
}

static void print_bme_gas_debug(void) {
    struct bme680_min_debug debug;
    char gas_text[20] = "ERR";
    float temp = 0.0f, hum = 0.0f, pres = 0.0f;
    uint32_t gas_ohms = 0;
    bool gas_valid = false;
    bool have_env = read_environment(&temp, &hum, &pres, &gas_ohms, &gas_valid);

    if (gas_valid) {
        snprintf(gas_text, sizeof(gas_text), "%luohm", (unsigned long)gas_ohms);
    }

    if (!bme680_get_last_debug(&debug)) {
        printf("[bme-gas] no-debug have_env=%u\n", have_env ? 1U : 0U);
        return;
    }

    printf("[bme-gas] have_env=%u addr=0x%02x status=0x%02x gas_msb=0x%02x gas_lsb=0x%02x adc=%u range=%u gas_valid=%u heatr_stab=%u ready=%u gas=%s temp=%.2fC hum=%.2f%% press=%.2fPa\n",
           have_env ? 1U : 0U, debug.addr, debug.status, debug.gas_msb, debug.gas_lsb,
           (unsigned)debug.adc_gas_res, (unsigned)debug.gas_range,
           debug.gas_valid ? 1U : 0U, debug.heatr_stab ? 1U : 0U,
           (debug.gas_valid && debug.heatr_stab) ? 1U : 0U, gas_text, temp, hum, pres);
}

static uint8_t frame_checksum(uint8_t type, uint8_t len, const uint8_t *payload) {
    uint8_t checksum = type ^ len;

    for (uint8_t i = 0; i < len; i++) {
        checksum ^= payload[i];
    }

    return checksum;
}

static void send_frame(uint8_t type, const char *payload) {
    size_t payload_len = strlen(payload);
    uint8_t checksum;

    if (payload_len > FRAME_MAX_PAYLOAD) {
        payload_len = FRAME_MAX_PAYLOAD;
    }

    checksum = frame_checksum(type, (uint8_t)payload_len, (const uint8_t *)payload);
    uart_putc_raw(LINK_UART_ID, FRAME_SOF0);
    uart_putc_raw(LINK_UART_ID, FRAME_SOF1);
    uart_putc_raw(LINK_UART_ID, type);
    uart_putc_raw(LINK_UART_ID, (uint8_t)payload_len);
    for (size_t i = 0; i < payload_len; i++) {
        uart_putc_raw(LINK_UART_ID, (uint8_t)payload[i]);
    }
    uart_putc_raw(LINK_UART_ID, checksum);

}

static void send_line(const char *line) {
    char payload[FRAME_MAX_PAYLOAD + 1];
    size_t len = strlen(line);

    while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) {
        len--;
    }
    if (len > FRAME_MAX_PAYLOAD) {
        len = FRAME_MAX_PAYLOAD;
    }

    memcpy(payload, line, len);
    payload[len] = '\0';

    if (reply_as_frame) {
        send_frame(reply_frame_type, payload);
    } else {
        uart_puts(LINK_UART_ID, line);
    }
}

static void send_base_bootsel_request(void) {
    send_frame(CMD_BASE_BOOTSEL, "BOOTSEL");
    printf("[maint] requested base BOOTSEL\n");
}

static void handle_usb_command(const char *cmd) {
    if (strcmp(cmd, "BASE_BOOTSEL") == 0 || strcmp(cmd, "BASE BOOTSEL") == 0) {
        send_base_bootsel_request();
    } else if (strcmp(cmd, "I2C_SCAN") == 0) {
        print_i2c_scan();
    } else if (strcmp(cmd, "BME_PROBE") == 0) {
        print_bme_probe();
    } else if (strcmp(cmd, "BME_GAS_DEBUG") == 0) {
        print_bme_gas_debug();
    } else if (strcmp(cmd, "PING") == 0) {
        printf("[maint] PONG\n");
    } else if (cmd[0] != '\0') {
        printf("[maint] unknown usb command: %s\n", cmd);
    }
}

static void process_usb_console(void) {
    static char cmd[USB_CMD_MAX_LEN];
    static size_t cmd_len;

    while (true) {
        int ch = getchar_timeout_us(0);

        if (ch == PICO_ERROR_TIMEOUT) {
            return;
        }
        if (ch == '\r') {
            continue;
        }
        if (ch == '\n') {
            cmd[cmd_len] = '\0';
            handle_usb_command(cmd);
            cmd_len = 0;
        } else if (cmd_len < (sizeof(cmd) - 1)) {
            cmd[cmd_len++] = (char)ch;
        } else {
            cmd_len = 0;
        }
    }
}

static void handle_read_all(void) {
    char out[160];
    char gas_text[16] = "ERR";
    float temp = 0.0f, hum = 0.0f, pres = 0.0f;
    uint32_t gas_ohms = 0;
    uint16_t light_raw = 0;
    uint16_t force = read_force_raw();
    bool have_bme;
    bool have_veml;
    bool gas_valid = false;
    int n;

    update_force_event(force);
    have_bme = read_environment(&temp, &hum, &pres, &gas_ohms, &gas_valid);
    have_veml = read_light(&light_raw);
    if (gas_valid) {
        snprintf(gas_text, sizeof(gas_text), "%lu", (unsigned long)gas_ohms);
    }

    if (have_bme && have_veml) {
        n = snprintf(out, sizeof(out),
                     "TEMP=%.2f,HUM=%.2f,PRESS=%.2f,GAS=%s,LIGHT=%u,FORCE=%u,EVENT=%u\n",
                     temp, hum, pres / 100.0f, gas_text, (unsigned)light_raw, (unsigned)force,
                     event_latched ? 1U : 0U);
    } else if (have_bme) {
        n = snprintf(out, sizeof(out),
                     "TEMP=%.2f,HUM=%.2f,PRESS=%.2f,GAS=%s,LIGHT=ERR,FORCE=%u,EVENT=%u\n",
                     temp, hum, pres / 100.0f, gas_text, (unsigned)force, event_latched ? 1U : 0U);
    } else if (have_veml) {
        n = snprintf(out, sizeof(out),
                     "TEMP=ERR,HUM=ERR,PRESS=ERR,GAS=ERR,LIGHT=%u,FORCE=%u,EVENT=%u\n",
                     (unsigned)light_raw, (unsigned)force, event_latched ? 1U : 0U);
    } else {
        n = snprintf(out, sizeof(out),
                     "TEMP=ERR,HUM=ERR,PRESS=ERR,GAS=ERR,LIGHT=ERR,FORCE=%u,EVENT=%u\n",
                     (unsigned)force, event_latched ? 1U : 0U);
    }

    if (n > 0) {
        send_line(out);
    }
}

static void handle_command(const char *cmd) {
    char out[96];
    uint32_t value;

    if (strcmp(cmd, "PING") == 0) {
        send_line("PONG\n");
    } else if (strcmp(cmd, "READ_FORCE") == 0) {
        uint16_t force = read_force_raw();
        update_force_event(force);
        snprintf(out, sizeof(out), "FORCE=%u,EVENT=%u\n", (unsigned)force, event_latched ? 1U : 0U);
        send_line(out);
    } else if (strcmp(cmd, "READ_ALL") == 0) {
        handle_read_all();
    } else if (strcmp(cmd, "READ_ENVIRONMENT") == 0) {
        char gas_text[16] = "ERR";
        float temp = 0.0f, hum = 0.0f, pres = 0.0f;
        uint32_t gas_ohms = 0;
        bool gas_valid = false;
        if (read_environment(&temp, &hum, &pres, &gas_ohms, &gas_valid)) {
            if (gas_valid) {
                snprintf(gas_text, sizeof(gas_text), "%lu", (unsigned long)gas_ohms);
            }
            snprintf(out, sizeof(out), "TEMP=%.2f,HUM=%.2f,PRESS=%.2f,GAS=%s\n",
                     temp, hum, pres / 100.0f, gas_text);
        } else {
            snprintf(out, sizeof(out), "TEMP=ERR,HUM=ERR,PRESS=ERR,GAS=ERR\n");
        }
        send_line(out);
    } else if (strcmp(cmd, "READ_LIGHT") == 0) {
        uint16_t light_raw = 0;
        if (read_light(&light_raw)) {
            snprintf(out, sizeof(out), "LIGHT=%u\n", (unsigned)light_raw);
        } else {
            snprintf(out, sizeof(out), "LIGHT=ERR\n");
        }
        send_line(out);
    } else if (strcmp(cmd, "READ_EVENT") == 0) {
        uint16_t force = read_force_raw();
        uint32_t age_ms = event_latched ? to_ms_since_boot(get_absolute_time()) - event_time_ms : 0;
        update_force_event(force);
        snprintf(out, sizeof(out), "EVENT=%u,FORCE=%u,EVENT_FORCE=%u,AGE_MS=%lu,THRESH=%u\n",
                 event_latched ? 1U : 0U, (unsigned)force, (unsigned)event_force,
                 (unsigned long)age_ms, (unsigned)force_threshold);
        send_line(out);
    } else if (strcmp(cmd, "CLEAR_EVENT") == 0) {
        event_latched = false;
        event_force = 0;
        event_time_ms = 0;
        update_event_irq_pin();
        printf("[event] cleared\n");
        send_line("OK,CLEAR_EVENT\n");
    } else if (strcmp(cmd, "READ_CONFIG") == 0) {
        snprintf(out, sizeof(out), "SAMPLING_MS=%lu,FORCE_THRESHOLD=%u,INTERRUPT=%u\n",
                 (unsigned long)sampling_interval_ms, (unsigned)force_threshold,
                 interrupt_enabled ? 1U : 0U);
        send_line(out);
    } else if (parse_u32_arg(cmd, "SET_SAMPLING_RATE=", &value)) {
        if (value < MIN_SAMPLING_MS) {
            value = MIN_SAMPLING_MS;
        }
        sampling_interval_ms = value;
        printf("[config] sampling_ms=%lu\n", (unsigned long)sampling_interval_ms);
        snprintf(out, sizeof(out), "OK,SAMPLING_MS=%lu\n", (unsigned long)sampling_interval_ms);
        send_line(out);
    } else if (parse_u32_arg(cmd, "SET_FORCE_THRESHOLD=", &value)) {
        if (value > 4095) {
            value = 4095;
        }
        force_threshold = (uint16_t)value;
        force_was_high = latest_force >= force_threshold;
        printf("[config] force_threshold=%u\n", (unsigned)force_threshold);
        snprintf(out, sizeof(out), "OK,FORCE_THRESHOLD=%u\n", (unsigned)force_threshold);
        send_line(out);
    } else if (parse_u32_arg(cmd, "ENABLE_INTERRUPT=", &value)) {
        interrupt_enabled = value != 0;
        update_event_irq_pin();
        printf("[config] interrupt=%u\n", interrupt_enabled ? 1U : 0U);
        snprintf(out, sizeof(out), "OK,INTERRUPT=%u\n", interrupt_enabled ? 1U : 0U);
        send_line(out);
    } else if (cmd[0] != '\0') {
        snprintf(out, sizeof(out), "ERR,UNKNOWN=%s\n", cmd);
        send_line(out);
    }
}

static bool feed_frame_byte(uint8_t byte,
                            enum frame_rx_state *state,
                            uint8_t *type,
                            uint8_t *len,
                            uint8_t *payload,
                            uint8_t *pos,
                            uint8_t *checksum) {
    switch (*state) {
    case FRAME_WAIT_SOF0:
        if (byte == FRAME_SOF0) {
            *state = FRAME_WAIT_SOF1;
        }
        break;
    case FRAME_WAIT_SOF1:
        *state = (byte == FRAME_SOF1) ? FRAME_READ_TYPE : FRAME_WAIT_SOF0;
        break;
    case FRAME_READ_TYPE:
        *type = byte;
        *checksum = byte;
        *state = FRAME_READ_LEN;
        break;
    case FRAME_READ_LEN:
        *len = byte;
        *pos = 0;
        *checksum ^= byte;
        if (*len > FRAME_MAX_PAYLOAD) {
            *state = FRAME_WAIT_SOF0;
        } else {
            *state = (*len == 0) ? FRAME_READ_CHECKSUM : FRAME_READ_PAYLOAD;
        }
        break;
    case FRAME_READ_PAYLOAD:
        payload[*pos] = byte;
        *checksum ^= byte;
        (*pos)++;
        if (*pos >= *len) {
            *state = FRAME_READ_CHECKSUM;
        }
        break;
    case FRAME_READ_CHECKSUM:
        *state = FRAME_WAIT_SOF0;
        return byte == *checksum;
    }

    return false;
}

static void handle_frame_command(uint8_t type, const uint8_t *payload, uint8_t len) {
    char cmd[96];
    int n = -1;

    switch (type) {
    case CMD_PING:
        n = snprintf(cmd, sizeof(cmd), "PING");
        break;
    case CMD_READ_FORCE:
        n = snprintf(cmd, sizeof(cmd), "READ_FORCE");
        break;
    case CMD_READ_ALL:
        n = snprintf(cmd, sizeof(cmd), "READ_ALL");
        break;
    case CMD_READ_ENVIRONMENT:
        n = snprintf(cmd, sizeof(cmd), "READ_ENVIRONMENT");
        break;
    case CMD_READ_LIGHT:
        n = snprintf(cmd, sizeof(cmd), "READ_LIGHT");
        break;
    case CMD_READ_EVENT:
        n = snprintf(cmd, sizeof(cmd), "READ_EVENT");
        break;
    case CMD_CLEAR_EVENT:
        n = snprintf(cmd, sizeof(cmd), "CLEAR_EVENT");
        break;
    case CMD_READ_CONFIG:
        n = snprintf(cmd, sizeof(cmd), "READ_CONFIG");
        break;
    case CMD_SET_SAMPLING_RATE:
        n = snprintf(cmd, sizeof(cmd), "SET_SAMPLING_RATE=%.*s", len, payload);
        break;
    case CMD_SET_FORCE_THRESHOLD:
        n = snprintf(cmd, sizeof(cmd), "SET_FORCE_THRESHOLD=%.*s", len, payload);
        break;
    case CMD_ENABLE_INTERRUPT:
        n = snprintf(cmd, sizeof(cmd), "ENABLE_INTERRUPT=%.*s", len, payload);
        break;
    default:
        n = snprintf(cmd, sizeof(cmd), "UNKNOWN_FRAME_0x%02x", type);
        break;
    }

    if (n > 0) {
        printf("[link-rx] type=0x%02x cmd=%s\n", type, cmd);
        reply_as_frame = true;
        reply_frame_type = type | FRAME_RESPONSE_FLAG;
        handle_command(cmd);
        reply_as_frame = false;
    }
}

int main(void) {
    stdio_init_all();

    sleep_ms(2000);
    printf("sensor-node ready\n");

    uart_init(LINK_UART_ID, LINK_BAUDRATE);
    gpio_set_function(LINK_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(LINK_UART_RX_PIN, GPIO_FUNC_UART);

    adc_init();
    adc_gpio_init(FSR_ADC_GPIO);
    adc_select_input(FSR_ADC_INPUT);

    i2c_init(SENSOR_I2C, 100 * 1000);
    gpio_set_function(SENSOR_I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SENSOR_I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SENSOR_I2C_SDA_PIN);
    gpio_pull_up(SENSOR_I2C_SCL_PIN);

    gpio_init(EVENT_IRQ_PIN);
    gpio_set_dir(EVENT_IRQ_PIN, GPIO_OUT);
    update_event_irq_pin();

    char rx_buf[80];
    size_t rx_len = 0;
    enum frame_rx_state frame_state = FRAME_WAIT_SOF0;
    uint8_t frame_type = 0;
    uint8_t frame_len = 0;
    uint8_t frame_payload[FRAME_MAX_PAYLOAD + 1];
    uint8_t frame_pos = 0;
    uint8_t frame_crc = 0;
    uint32_t last_sensor_ms = 0;
    uint32_t last_event_sample_ms = 0;

    while (true) {
        process_usb_console();

        while (uart_is_readable(LINK_UART_ID)) {
            int ch = uart_getc(LINK_UART_ID);

            if (frame_state != FRAME_WAIT_SOF0 || (uint8_t)ch == FRAME_SOF0) {
                if (feed_frame_byte((uint8_t)ch, &frame_state, &frame_type, &frame_len,
                                    frame_payload, &frame_pos, &frame_crc)) {
                    frame_payload[frame_len] = '\0';
                    handle_frame_command(frame_type, frame_payload, frame_len);
                }
                continue;
            }

            if (ch == '\r') {
                continue;
            }
            if (ch == '\n') {
                rx_buf[rx_len] = '\0';
                handle_command(rx_buf);
                rx_len = 0;
            } else if (rx_len < (sizeof(rx_buf) - 1)) {
                rx_buf[rx_len++] = (char)ch;
            } else {
                rx_len = 0;
            }
        }

        uint32_t now_ms = to_ms_since_boot(get_absolute_time());
        if (now_ms - last_event_sample_ms >= FORCE_EVENT_SAMPLE_MS) {
            last_event_sample_ms = now_ms;
            update_force_event(read_force_raw());
        }

        if (now_ms - last_sensor_ms >= sampling_interval_ms) {
            char gas_text[20] = "ERR";
            float temp = 0.0f, hum = 0.0f, pres = 0.0f;
            uint32_t gas_ohms = 0;
            uint16_t light_raw = 0;
            bool have_env;
            bool have_light;
            bool gas_valid = false;
            last_sensor_ms = now_ms;

            have_env = read_environment(&temp, &hum, &pres, &gas_ohms, &gas_valid);
            have_light = read_light(&light_raw);
            if (gas_valid) {
                snprintf(gas_text, sizeof(gas_text), "%luohm", (unsigned long)gas_ohms);
            }

            if (have_env && have_light) {
                printf("[sample] force=%u event=%u thresh=%u temp=%.2fC hum=%.2f%% press=%.2fPa gas=%s light=%u\n",
                       (unsigned)latest_force, event_latched ? 1U : 0U, (unsigned)force_threshold,
                       temp, hum, pres, gas_text, (unsigned)light_raw);
            } else if (have_env) {
                printf("[sample] force=%u event=%u thresh=%u temp=%.2fC hum=%.2f%% press=%.2fPa gas=%s light=ERR\n",
                       (unsigned)latest_force, event_latched ? 1U : 0U, (unsigned)force_threshold,
                       temp, hum, pres, gas_text);
            } else if (have_light) {
                printf("[sample] force=%u event=%u thresh=%u env=ERR light=%u\n",
                       (unsigned)latest_force, event_latched ? 1U : 0U, (unsigned)force_threshold,
                       (unsigned)light_raw);
            } else {
                printf("[sample] force=%u event=%u thresh=%u env=ERR light=ERR\n",
                       (unsigned)latest_force, event_latched ? 1U : 0U, (unsigned)force_threshold);
            }
        }

        sleep_ms(1);
    }

    return 0;
}
