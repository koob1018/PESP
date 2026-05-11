#pragma once
#include "hardware/i2c.h"
#include <stdbool.h>
#include <stdint.h>

struct bme680_min_debug {
    bool valid;
    int addr;
    uint8_t status;
    uint8_t gas_msb;
    uint8_t gas_lsb;
    uint16_t adc_gas_res;
    uint8_t gas_range;
    bool gas_valid;
    bool heatr_stab;
};

bool bme680_init_min(i2c_inst_t *i2c, int addr);
bool bme680_read_min(i2c_inst_t *i2c, int addr, float *temperature_c, float *humidity_pct, float *pressure_pa);
bool bme680_read_all_min(i2c_inst_t *i2c, int addr, float *temperature_c, float *humidity_pct,
                         float *pressure_pa, uint32_t *gas_resistance_ohms, bool *gas_valid);
bool bme680_get_last_debug(struct bme680_min_debug *debug);
