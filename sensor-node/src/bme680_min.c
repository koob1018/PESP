#include "bme680_min.h"

#include "pico/stdlib.h"

#include <stdint.h>

#define BME680_CHIP_ID 0x61

#define BME680_LEN_COEFF_ALL 42
#define BME680_LEN_COEFF1 23
#define BME680_LEN_COEFF2 14
#define BME680_LEN_COEFF3 5

#define BME680_REG_COEFF3 0x00
#define BME680_REG_MEAS_STATUS 0x1D
#define BME680_REG_FIELD0 0x1F
#define BME680_REG_RES_HEAT0 0x5A
#define BME680_REG_GAS_WAIT0 0x64
#define BME680_REG_CTRL_GAS_1 0x71
#define BME680_REG_CTRL_HUM 0x72
#define BME680_REG_CTRL_MEAS 0x74
#define BME680_REG_CONFIG 0x75
#define BME680_REG_COEFF1 0x8A
#define BME680_REG_CHIP_ID 0xD0
#define BME680_REG_COEFF2 0xE1

#define BME680_MSK_NEW_DATA 0x80
#define BME680_MSK_GAS_RANGE 0x0F
#define BME680_MSK_GAS_VALID 0x20
#define BME680_MSK_HEATR_STAB 0x10
#define BME680_MSK_RH_RANGE 0x30
#define BME680_MSK_RANGE_SW_ERR 0xF0

#define BME680_CTRL_HUM_VAL 0x01
#define BME680_CONFIG_VAL 0x00
#define BME680_CTRL_GAS_1_VAL 0x10
#define BME680_CTRL_MEAS_FORCED 0x25
#define BME680_HEATR_TEMP_C 320
#define BME680_HEATR_DUR_MS 197

#define BME680_CONCAT_BYTES(msb, lsb) (((uint16_t)(msb) << 8) | (uint16_t)(lsb))

struct bme680_state {
    bool valid;
    int addr;

    uint16_t par_h1;
    uint16_t par_h2;
    int8_t par_h3;
    int8_t par_h4;
    int8_t par_h5;
    uint8_t par_h6;
    int8_t par_h7;
    int8_t par_gh1;
    int16_t par_gh2;
    int8_t par_gh3;
    uint16_t par_t1;
    int16_t par_t2;
    int8_t par_t3;
    uint16_t par_p1;
    int16_t par_p2;
    int8_t par_p3;
    int16_t par_p4;
    int16_t par_p5;
    int8_t par_p6;
    int8_t par_p7;
    int16_t par_p8;
    int16_t par_p9;
    uint8_t par_p10;
    uint8_t res_heat_range;
    int8_t res_heat_val;
    int8_t range_sw_err;
    int32_t t_fine;
};

static struct bme680_state g_bme680;
static struct bme680_min_debug g_bme680_debug;

static bool bme680_read_regs(i2c_inst_t *i2c, int addr, uint8_t reg, uint8_t *buf, size_t len) {
    if (i2c_write_blocking(i2c, addr, &reg, 1, true) < 0) {
        return false;
    }
    if (i2c_read_blocking(i2c, addr, buf, (size_t)len, false) < 0) {
        return false;
    }
    return true;
}

static bool bme680_write_reg(i2c_inst_t *i2c, int addr, uint8_t reg, uint8_t value) {
    uint8_t buf[2] = { reg, value };
    return i2c_write_blocking(i2c, addr, buf, 2, false) >= 0;
}

static void bme680_calc_temp(struct bme680_state *state, uint32_t adc_temp, float *temperature_c) {
    int64_t var1, var2, var3;
    int32_t calc_temp;

    var1 = ((int32_t)adc_temp >> 3) - ((int32_t)state->par_t1 << 1);
    var2 = (var1 * (int32_t)state->par_t2) >> 11;
    var3 = ((var1 >> 1) * (var1 >> 1)) >> 12;
    var3 = (var3 * ((int32_t)state->par_t3 << 4)) >> 14;
    state->t_fine = (int32_t)(var2 + var3);
    calc_temp = (int32_t)(((state->t_fine * 5) + 128) >> 8);
    *temperature_c = (float)calc_temp / 100.0f;
}

static void bme680_calc_press(struct bme680_state *state, uint32_t adc_press, float *pressure_pa) {
    int32_t var1, var2, var3, calc_press;

    var1 = (state->t_fine >> 1) - 64000;
    var2 = ((((var1 >> 2) * (var1 >> 2)) >> 11) * (int32_t)state->par_p6) >> 2;
    var2 += (var1 * (int32_t)state->par_p5) << 1;
    var2 = (var2 >> 2) + ((int32_t)state->par_p4 << 16);
    var1 = (((((var1 >> 2) * (var1 >> 2)) >> 13) * ((int32_t)state->par_p3 << 5)) >> 3) +
           (((int32_t)state->par_p2 * var1) >> 1);
    var1 >>= 18;
    var1 = ((32768 + var1) * (int32_t)state->par_p1) >> 15;

    if (var1 == 0) {
        *pressure_pa = 0.0f;
        return;
    }

    calc_press = 1048576 - (int32_t)adc_press;
    calc_press = (int32_t)((calc_press - (var2 >> 12)) * 3125U);
    if (calc_press >= (int32_t)0x40000000) {
        calc_press = (calc_press / var1) << 1;
    } else {
        calc_press = (calc_press << 1) / var1;
    }

    var1 = ((int32_t)state->par_p9 *
            (int32_t)(((calc_press >> 3) * (calc_press >> 3)) >> 13)) >> 12;
    var2 = ((int32_t)(calc_press >> 2) * (int32_t)state->par_p8) >> 13;
    var3 = ((int32_t)(calc_press >> 8) * (int32_t)(calc_press >> 8) *
            (int32_t)(calc_press >> 8) * (int32_t)state->par_p10) >> 17;

    calc_press += (var1 + var2 + var3 + ((int32_t)state->par_p7 << 7)) >> 4;
    *pressure_pa = (float)calc_press;
}

static void bme680_calc_humidity(struct bme680_state *state, uint16_t adc_humidity, float *humidity_pct) {
    int32_t var1, var2_1, var2_2, var2, var3, var4, var5, var6;
    int32_t temp_scaled, calc_hum;

    temp_scaled = ((state->t_fine * 5) + 128) >> 8;
    var1 = (int32_t)adc_humidity - ((int32_t)state->par_h1 * 16) -
           (((temp_scaled * (int32_t)state->par_h3) / 100) >> 1);
    var2_1 = (int32_t)state->par_h2;
    var2_2 = ((temp_scaled * (int32_t)state->par_h4) / 100) +
             (((temp_scaled * ((temp_scaled * (int32_t)state->par_h5) / 100)) >> 6) / 100) +
             (1 << 14);
    var2 = (var2_1 * var2_2) >> 10;
    var3 = var1 * var2;
    var4 = ((int32_t)state->par_h6 << 7) + ((temp_scaled * (int32_t)state->par_h7) / 100);
    var4 >>= 4;
    var5 = ((var3 >> 14) * (var3 >> 14)) >> 10;
    var6 = (var4 * var5) >> 1;
    calc_hum = (((var3 + var6) >> 10) * 1000) >> 12;

    if (calc_hum > 100000) {
        calc_hum = 100000;
    } else if (calc_hum < 0) {
        calc_hum = 0;
    }

    *humidity_pct = (float)calc_hum / 1000.0f;
}

static uint32_t bme680_calc_gas_resistance(const struct bme680_state *state,
                                           uint8_t gas_range,
                                           uint16_t adc_gas_res) {
    int64_t var1, var3;
    uint64_t var2;

    static const uint32_t look_up1[16] = {
        2147483647, 2147483647, 2147483647, 2147483647,
        2147483647, 2126008810, 2147483647, 2130303777,
        2147483647, 2147483647, 2143188679, 2136746228,
        2147483647, 2126008810, 2147483647, 2147483647
    };
    static const uint32_t look_up2[16] = {
        4096000000, 2048000000, 1024000000, 512000000,
        255744255, 127110228, 64000000, 32258064,
        16016016, 8000000, 4000000, 2000000,
        1000000, 500000, 250000, 125000
    };

    var1 = (int64_t)((1340 + (5 * (int64_t)state->range_sw_err)) *
                     (int64_t)look_up1[gas_range]) >> 16;
    var2 = (((int64_t)adc_gas_res << 15) - (int64_t)16777216) + var1;
    var3 = ((int64_t)look_up2[gas_range] * var1) >> 9;

    return (uint32_t)((var3 + ((int64_t)var2 >> 1)) / (int64_t)var2);
}

static uint8_t bme680_calc_res_heat(const struct bme680_state *state, uint16_t heatr_temp) {
    int32_t var1, var2, var3, var4, var5;
    int32_t heatr_res_x100;
    const int32_t amb_temp = 25;

    if (heatr_temp > 400) {
        heatr_temp = 400;
    }

    var1 = ((amb_temp * state->par_gh3) / 1000) * 256;
    var2 = (state->par_gh1 + 784) *
           (((((state->par_gh2 + 154009) * (int32_t)heatr_temp * 5) / 100) + 3276800) / 10);
    var3 = var1 + (var2 / 2);
    var4 = var3 / ((int32_t)state->res_heat_range + 4);
    var5 = (131 * (int32_t)state->res_heat_val) + 65536;
    heatr_res_x100 = ((var4 / var5) - 250) * 34;
    return (uint8_t)((heatr_res_x100 + 50) / 100);
}

static uint8_t bme680_calc_gas_wait(uint16_t dur_ms) {
    uint8_t factor = 0;

    if (dur_ms >= 0x0FC0) {
        return 0xFF;
    }

    while (dur_ms > 0x3F) {
        dur_ms /= 4;
        factor++;
    }

    return (uint8_t)(dur_ms + (factor * 64));
}

static bool bme680_read_calibration(i2c_inst_t *i2c, int addr, struct bme680_state *state) {
    uint8_t buff[BME680_LEN_COEFF_ALL];

    if (!bme680_read_regs(i2c, addr, BME680_REG_COEFF1, buff, BME680_LEN_COEFF1)) {
        return false;
    }
    if (!bme680_read_regs(i2c, addr, BME680_REG_COEFF2, &buff[BME680_LEN_COEFF1], BME680_LEN_COEFF2)) {
        return false;
    }
    if (!bme680_read_regs(i2c, addr, BME680_REG_COEFF3, &buff[BME680_LEN_COEFF1 + BME680_LEN_COEFF2], BME680_LEN_COEFF3)) {
        return false;
    }

    state->par_t1 = BME680_CONCAT_BYTES(buff[32], buff[31]);
    state->par_t2 = (int16_t)BME680_CONCAT_BYTES(buff[1], buff[0]);
    state->par_t3 = (int8_t)buff[2];

    state->par_p1 = BME680_CONCAT_BYTES(buff[5], buff[4]);
    state->par_p2 = (int16_t)BME680_CONCAT_BYTES(buff[7], buff[6]);
    state->par_p3 = (int8_t)buff[8];
    state->par_p4 = (int16_t)BME680_CONCAT_BYTES(buff[11], buff[10]);
    state->par_p5 = (int16_t)BME680_CONCAT_BYTES(buff[13], buff[12]);
    state->par_p6 = (int8_t)buff[15];
    state->par_p7 = (int8_t)buff[14];
    state->par_p8 = (int16_t)BME680_CONCAT_BYTES(buff[19], buff[18]);
    state->par_p9 = (int16_t)BME680_CONCAT_BYTES(buff[21], buff[20]);
    state->par_p10 = buff[22];

    state->par_h1 = (uint16_t)(((uint16_t)buff[25] << 4) | (buff[24] & 0x0F));
    state->par_h2 = (uint16_t)(((uint16_t)buff[23] << 4) | (buff[24] >> 4));
    state->par_h3 = (int8_t)buff[26];
    state->par_h4 = (int8_t)buff[27];
    state->par_h5 = (int8_t)buff[28];
    state->par_h6 = buff[29];
    state->par_h7 = (int8_t)buff[30];

    state->par_gh1 = (int8_t)buff[35];
    state->par_gh2 = (int16_t)BME680_CONCAT_BYTES(buff[34], buff[33]);
    state->par_gh3 = (int8_t)buff[36];

    state->res_heat_val = (int8_t)buff[37];
    state->res_heat_range = (buff[39] & BME680_MSK_RH_RANGE) >> 4;
    state->range_sw_err = (int8_t)(buff[41] & BME680_MSK_RANGE_SW_ERR) / 16;

    return true;
}

bool bme680_init_min(i2c_inst_t *i2c, int addr) {
    uint8_t chip_id = 0;

    if (g_bme680.valid && g_bme680.addr == addr) {
        return true;
    }

    if (!bme680_read_regs(i2c, addr, BME680_REG_CHIP_ID, &chip_id, 1)) {
        return false;
    }
    if (chip_id != BME680_CHIP_ID) {
        return false;
    }

    struct bme680_state next = { 0 };
    next.addr = addr;

    if (!bme680_read_calibration(i2c, addr, &next)) {
        return false;
    }
    if (!bme680_write_reg(i2c, addr, BME680_REG_CTRL_HUM, BME680_CTRL_HUM_VAL)) {
        return false;
    }
    if (!bme680_write_reg(i2c, addr, BME680_REG_CONFIG, BME680_CONFIG_VAL)) {
        return false;
    }
    if (!bme680_write_reg(i2c, addr, BME680_REG_CTRL_GAS_1, BME680_CTRL_GAS_1_VAL)) {
        return false;
    }
    if (!bme680_write_reg(i2c, addr, BME680_REG_RES_HEAT0, bme680_calc_res_heat(&next, BME680_HEATR_TEMP_C))) {
        return false;
    }
    if (!bme680_write_reg(i2c, addr, BME680_REG_GAS_WAIT0, bme680_calc_gas_wait(BME680_HEATR_DUR_MS))) {
        return false;
    }

    next.valid = true;
    g_bme680 = next;
    return true;
}

bool bme680_read_all_min(i2c_inst_t *i2c, int addr, float *temperature_c, float *humidity_pct,
                         float *pressure_pa, uint32_t *gas_resistance_ohms, bool *gas_valid) {
    uint8_t status = 0;
    uint8_t data[13];
    int retries = 0;
    uint32_t adc_press, adc_temp;
    uint16_t adc_hum;
    uint16_t adc_gas_res;
    uint8_t gas_range;
    bool gas_ready;
    struct bme680_state *state = &g_bme680;

    if (temperature_c == NULL || humidity_pct == NULL || pressure_pa == NULL) {
        return false;
    }
    if (gas_resistance_ohms != NULL) {
        *gas_resistance_ohms = 0;
    }
    if (gas_valid != NULL) {
        *gas_valid = false;
    }
    if (!bme680_init_min(i2c, addr)) {
        return false;
    }

    if (!bme680_write_reg(i2c, addr, BME680_REG_CTRL_MEAS, BME680_CTRL_MEAS_FORCED)) {
        return false;
    }

    do {
        sleep_ms(1);
        if (!bme680_read_regs(i2c, addr, BME680_REG_MEAS_STATUS, &status, 1)) {
            return false;
        }
        retries++;
    } while (((status & BME680_MSK_NEW_DATA) == 0) && retries < 250);

    if ((status & BME680_MSK_NEW_DATA) == 0) {
        return false;
    }
    if (!bme680_read_regs(i2c, addr, BME680_REG_FIELD0, data, sizeof(data))) {
        return false;
    }

    adc_press = ((uint32_t)data[0] << 12) | ((uint32_t)data[1] << 4) | (data[2] >> 4);
    adc_temp = ((uint32_t)data[3] << 12) | ((uint32_t)data[4] << 4) | (data[5] >> 4);
    adc_hum = ((uint16_t)data[6] << 8) | data[7];
    adc_gas_res = (((uint16_t)data[11] << 8) | data[12]) >> 6;
    gas_range = data[12] & BME680_MSK_GAS_RANGE;
    gas_ready = (data[12] & (BME680_MSK_GAS_VALID | BME680_MSK_HEATR_STAB)) ==
                (BME680_MSK_GAS_VALID | BME680_MSK_HEATR_STAB);
    g_bme680_debug = (struct bme680_min_debug) {
        .valid = true,
        .addr = addr,
        .status = status,
        .gas_msb = data[11],
        .gas_lsb = data[12],
        .adc_gas_res = adc_gas_res,
        .gas_range = gas_range,
        .gas_valid = (data[12] & BME680_MSK_GAS_VALID) != 0,
        .heatr_stab = (data[12] & BME680_MSK_HEATR_STAB) != 0,
    };

    bme680_calc_temp(state, adc_temp, temperature_c);
    bme680_calc_press(state, adc_press, pressure_pa);
    bme680_calc_humidity(state, adc_hum, humidity_pct);
    if (gas_ready) {
        if (gas_resistance_ohms != NULL) {
            *gas_resistance_ohms = bme680_calc_gas_resistance(state, gas_range, adc_gas_res);
        }
        if (gas_valid != NULL) {
            *gas_valid = true;
        }
    }

    return (*temperature_c > -40.0f && *temperature_c < 85.0f &&
            *pressure_pa > 30000.0f && *pressure_pa < 120000.0f &&
            *humidity_pct >= 0.0f && *humidity_pct <= 100.0f);
}

bool bme680_read_min(i2c_inst_t *i2c, int addr, float *temperature_c, float *humidity_pct, float *pressure_pa) {
    return bme680_read_all_min(i2c, addr, temperature_c, humidity_pct, pressure_pa, NULL, NULL);
}

bool bme680_get_last_debug(struct bme680_min_debug *debug) {
    if (debug == NULL || !g_bme680_debug.valid) {
        return false;
    }

    *debug = g_bme680_debug;
    return true;
}
