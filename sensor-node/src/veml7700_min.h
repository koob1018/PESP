#pragma once
#include "hardware/i2c.h"
#include <stdbool.h>

bool veml7700_init_min(i2c_inst_t *i2c, int addr);
bool veml7700_read_raw(i2c_inst_t *i2c, int addr, uint16_t *raw);
