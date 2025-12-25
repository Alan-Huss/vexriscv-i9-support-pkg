#include "max3010x.h"
#include "i2c_driver.h"
#include "time_driver.h"

#include <stdint.h>
#include <stdbool.h>

/* Registradores */
#define REG_FIFO_DATA     0x07
#define REG_MODE_CONFIG   0x09
#define REG_SPO2_CONFIG   0x0A
#define REG_LED1_PA       0x0C
#define REG_LED2_PA       0x0D

#define MODE_RESET 0x40
#define MODE_SPO2  0x03

#define PEAK_TH     800
#define PEAK_HYST   300

/* ================= I2C ================= */

static bool write_reg(uint8_t addr, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = {reg, val};
    return bb_i2c_write(addr, buf, 2);
}

/* ================= INIT ================= */

bool max3010x_init(max3010x_ctx_t *ctx) {
    if (!ctx) return false;

    ctx->i2c_addr = MAX3010X_I2C_ADDR;
    ctx->bpm = 0;
    ctx->finger_detected = false;

    ctx->ir_idx = 0;
    ctx->ir_dc = 0;
    ctx->peak_high = false;
    ctx->rr_i = 0;
    ctx->last_peak_ms = 0;

    bb_i2c_init();
    delay_ms(10);

    write_reg(ctx->i2c_addr, REG_MODE_CONFIG, MODE_RESET);
    delay_ms(100);

    write_reg(ctx->i2c_addr, REG_MODE_CONFIG, MODE_SPO2);
    write_reg(ctx->i2c_addr, REG_SPO2_CONFIG, 0x27);
    write_reg(ctx->i2c_addr, REG_LED1_PA, 0x24);
    write_reg(ctx->i2c_addr, REG_LED2_PA, 0x24);

    return true;
}

/* ================= FIFO ================= */

bool max3010x_read_fifo(max3010x_ctx_t *ctx) {
    uint8_t raw[6];
    uint8_t reg = REG_FIFO_DATA;

    if (!bb_i2c_write(ctx->i2c_addr, &reg, 1)) return false;
    if (!bb_i2c_read(ctx->i2c_addr, raw, 6)) return false;

    ctx->red_value =
        ((uint32_t)raw[0] << 16) |
        ((uint32_t)raw[1] << 8) |
        raw[2];
    ctx->red_value &= 0x3FFFF;

    ctx->ir_value =
        ((uint32_t)raw[3] << 16) |
        ((uint32_t)raw[4] << 8) |
        raw[5];
    ctx->ir_value &= 0x3FFFF;

    ctx->finger_detected = (ctx->ir_value > 50000);
    return true;
}

/* ================= PPG ================= */

static uint32_t ir_filtered(max3010x_ctx_t *ctx, uint32_t v) {
    ctx->ir_buf[ctx->ir_idx++] = v;
    if (ctx->ir_idx >= IR_BUF) ctx->ir_idx = 0;

    uint64_t sum = 0;
    for (int i = 0; i < IR_BUF; i++) sum += ctx->ir_buf[i];
    return sum / IR_BUF;
}

static int32_t ir_ac(max3010x_ctx_t *ctx, uint32_t ir) {
    ctx->ir_dc = (ctx->ir_dc * 31 + ir) / 32;
    return (int32_t)ir - ctx->ir_dc;
}

static bool detect_peak(max3010x_ctx_t *ctx, int32_t ac) {
    if (!ctx->peak_high && ac > PEAK_TH) {
        ctx->peak_high = true;
        return true;
    }
    if (ctx->peak_high && ac < PEAK_TH - PEAK_HYST) {
        ctx->peak_high = false;
    }
    return false;
}

static uint32_t bpm_from_rr(max3010x_ctx_t *ctx, uint32_t dt) {
    if (dt < 300 || dt > 2000) return 0;

    ctx->rr[ctx->rr_i++] = dt;
    if (ctx->rr_i >= RR_BUF) ctx->rr_i = 0;

    uint32_t sum = 0;
    for (int i = 0; i < RR_BUF; i++) sum += ctx->rr[i];
    return 60000 / (sum / RR_BUF);
}

/* ================= UPDATE ================= */

void max3010x_update(max3010x_ctx_t *ctx, uint32_t elapsed_ms) {
    if (!ctx->finger_detected) {
        ctx->bpm = 0;
        return;
    }

    uint32_t ir_f = ir_filtered(ctx, ctx->ir_value);
    int32_t ac = ir_ac(ctx, ir_f);

    if (detect_peak(ctx, ac)) {
        uint32_t dt = elapsed_ms - ctx->last_peak_ms;
        ctx->last_peak_ms = elapsed_ms;

        uint32_t bpm = bpm_from_rr(ctx, dt);
        if (bpm > 30 && bpm < 200) ctx->bpm = bpm;
    }
}
