#include "veml7700_min.h"
#include <stdio.h>

/* Minimal VEML7700 init + raw read
 * Registers: CONFIG(0x00), ALS(0x04)
 */

static int i2c_write_reg16(i2c_inst_t *i2c, int addr, uint8_t reg, uint16_t val) {
    uint8_t buf[3];
    buf[0] = reg;
    buf[1] = val & 0xFF;
    buf[2] = (val >> 8) & 0xFF;
    return i2c_write_blocking(i2c, addr, buf, 3, false);
}

static int i2c_read_reg16(i2c_inst_t *i2c, int addr, uint8_t reg, uint16_t *val) {
    if (i2c_write_blocking(i2c, addr, &reg, 1, true) < 0) return -1;
    uint8_t buf[2];
    if (i2c_read_blocking(i2c, addr, buf, 2, false) < 0) return -1;
    *val = (uint16_t)buf[0] | ((uint16_t)buf[1] << 8);
    return 0;
}

bool veml7700_init_min(i2c_inst_t *i2c, int addr) {
    /* write default config: ALS gain = 1, IT = 100ms, power on */
    uint16_t config = 0x0000; // default
    if (i2c_write_reg16(i2c, addr, 0x00, config) < 0) return false;
    return true;
}

bool veml7700_read_raw(i2c_inst_t *i2c, int addr, uint16_t *raw) {
    uint16_t v;
    if (i2c_read_reg16(i2c, addr, 0x04, &v) < 0) return false;
    *raw = v;
    return true;
}
