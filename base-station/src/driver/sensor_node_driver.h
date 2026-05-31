#pragma once

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>

DEVICE_DECLARE(sensor_node_driver);

typedef void (*sensor_node_driver_log_sink_t)(const char *message, void *user_data);

struct sensor_node_driver_reading {
    bool valid;
    bool has_environment;
    int32_t temperature_centi_c;
    uint32_t humidity_milli_pct;
    uint32_t pressure_centi_hpa;
    bool has_gas;
    uint32_t gas_ohms;
    bool has_light;
    uint16_t light_raw;
    bool has_force;
    uint16_t force_raw;
    bool event_latched;
};

struct sensor_node_driver_event {
    bool valid;
    bool latched;
    uint16_t force_raw;
    uint16_t event_force_raw;
    uint16_t threshold_raw;
    uint32_t age_ms;
};

struct sensor_node_driver_config {
    bool valid;
    uint32_t sampling_ms;
    uint16_t force_threshold;
    bool interrupt_enabled;
    bool service_mode;
    uint16_t light_high_threshold;
    int32_t temp_high_centi_c;
    uint32_t humidity_high_milli_pct;
};

int sensor_node_driver_start(const struct device *dev);
void sensor_node_driver_process(const struct device *dev);
int sensor_node_driver_request_all(const struct device *dev);
int sensor_node_driver_request_force(const struct device *dev);
int sensor_node_driver_request_event(const struct device *dev);
int sensor_node_driver_clear_event(const struct device *dev);
int sensor_node_driver_set_force_threshold(const struct device *dev, uint16_t threshold);
int sensor_node_driver_set_sampling_interval(const struct device *dev, uint32_t sampling_ms);
int sensor_node_driver_enable_interrupt(const struct device *dev, bool enabled);
int sensor_node_driver_set_light_high(const struct device *dev, uint16_t threshold);
int sensor_node_driver_set_temp_high(const struct device *dev, const char *threshold_c);
int sensor_node_driver_set_humidity_high(const struct device *dev, const char *threshold_pct);
int sensor_node_driver_set_service_mode(const struct device *dev, bool enabled);
int sensor_node_driver_read_config(const struct device *dev);
bool sensor_node_driver_get_latest_reading(const struct device *dev, struct sensor_node_driver_reading *reading);
bool sensor_node_driver_get_latest_event(const struct device *dev, struct sensor_node_driver_event *event);
bool sensor_node_driver_get_config(const struct device *dev, struct sensor_node_driver_config *config);
void sensor_node_driver_set_logging_enabled(bool enabled);
void sensor_node_driver_set_log_sink(sensor_node_driver_log_sink_t sink, void *user_data);
