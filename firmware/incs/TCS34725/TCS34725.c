#include "TCS34725.h"
#include "i2c_driver.h"
#include "time_driver.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* Registradores */
#define COMMAND_BIT     0x80
#define REG_ENABLE      0x00
#define REG_ATIME       0x01
#define REG_CONTROL     0x0F
#define REG_ID          0x12
#define REG_CDATAL      0x14

/* ENABLE bits */
#define ENABLE_PON      0x01
#define ENABLE_AEN      0x02

/* ================= I2C helpers ================= */

static bool write8(uint8_t addr, uint8_t reg, uint8_t val) {
    uint8_t buf[2] = { COMMAND_BIT | reg, val };
    return bb_i2c_write(addr, buf, 2);
}

static bool read8(uint8_t addr, uint8_t reg, uint8_t *val) {
    uint8_t cmd = COMMAND_BIT | reg;
    if (!bb_i2c_write(addr, &cmd, 1)) return false;
    return bb_i2c_read(addr, val, 1);
}

static bool read16(uint8_t addr, uint8_t reg, uint16_t *val) {
    uint8_t cmd = COMMAND_BIT | reg;
    uint8_t buf[2];

    if (!bb_i2c_write(addr, &cmd, 1)) return false;
    if (!bb_i2c_read(addr, buf, 2)) return false;

    *val = (uint16_t)buf[1] << 8 | buf[0];
    return true;
}

/* ================= API ================= */

bool tcs34725_init(tcs34725_ctx_t *ctx, tcs34725_gain_t gain, tcs34725_integration_t integration) {

    uint16_t integration_ms = (256 - integration) * 2.4;

    delay_ms(integration_ms + 10);

    if (!ctx) return false;

    ctx->i2c_addr = TCS34725_I2C_ADDR;
    ctx->gain = gain;
    ctx->integration = integration;

    bb_i2c_init();
    delay_ms(10);

    uint8_t id;
    if (!read8(ctx->i2c_addr, REG_ID, &id)) {
        printf("Erro leitura ID TCS\n");
        return false;
    }

    write8(ctx->i2c_addr, REG_ENABLE, ENABLE_PON);
    delay_ms(10);

    write8(ctx->i2c_addr, REG_ENABLE, ENABLE_PON | ENABLE_AEN);

    write8(ctx->i2c_addr, REG_ATIME, integration);
    write8(ctx->i2c_addr, REG_CONTROL, gain);

    delay_ms(100);
    return true;
}


bool tcs34725_read_raw(tcs34725_ctx_t *ctx,
                       uint16_t *clear,
                       uint16_t *red,
                       uint16_t *green,
                       uint16_t *blue)
{
    if (!ctx) return false;

    if (!read16(ctx->i2c_addr, REG_CDATAL + 0, clear)) return false;
    if (!read16(ctx->i2c_addr, REG_CDATAL + 2, red))   return false;
    if (!read16(ctx->i2c_addr, REG_CDATAL + 4, green)) return false;
    if (!read16(ctx->i2c_addr, REG_CDATAL + 6, blue))  return false;

    return true;
}
