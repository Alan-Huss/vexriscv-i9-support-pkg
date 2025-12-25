// bh1750.c
// Driver BH1750 para LiteX usando bb_i2c

#include "bh1750.h"
#include "i2c_driver.h"
#include "time_driver.h"

#include <stdint.h>
#include <stdbool.h>

#define BH1750_POWER_ON   0x01
#define BH1750_RESET      0x07

bool bh1750_init(bh1750_ctx_t *ctx, uint8_t addr, bh1750_mode_t mode) {
    if (!ctx) {
        return false;
    }

    ctx->i2c_addr = addr;
    ctx->mode     = mode;
    ctx->lux_x100 = 0;

    bb_i2c_init();
    delay_ms(10);

    /* Power ON */
    uint8_t cmd = BH1750_POWER_ON;
    if (!bb_i2c_write(ctx->i2c_addr, &cmd, 1)) {
        return false;
    }

    delay_ms(10);

    /* Reset */
    cmd = BH1750_RESET;
    if (!bb_i2c_write(ctx->i2c_addr, &cmd, 1)) {
        return false;
    }

    delay_ms(10);

    /* Configura modo */
    cmd = (uint8_t)ctx->mode;
    if (!bb_i2c_write(ctx->i2c_addr, &cmd, 1)) {
        return false;
    }

    /* Tempo típico de conversão */
    delay_ms(180);

    return true;
}

bool bh1750_read(bh1750_ctx_t *ctx) {
    if (!ctx) {
        return false;
    }

    uint8_t rx[2];

    /* Leitura de 2 bytes */
    if (!bb_i2c_read(ctx->i2c_addr, rx, 2)) {
        return false;
    }

    uint16_t raw =
        ((uint16_t)rx[0] << 8) |
        rx[1];

    /*
     * Datasheet:
     * lux = raw / 1.2
     * Para evitar float:
     * lux_x100 = raw * 100 / 1.2 = raw * 1000 / 12
     */

    ctx->lux_x100 = ((uint32_t)raw * 1000UL) / 12UL;

    return true;
}
