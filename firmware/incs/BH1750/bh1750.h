// bh1750.h
// Driver BH1750 para LiteX (bb_i2c)

#ifndef BH1750_H
#define BH1750_H

#include <stdint.h>
#include <stdbool.h>

/* Endereços I2C possíveis */
#define BH1750_ADDR_LOW   0x23
#define BH1750_ADDR_HIGH  0x5C

/* Modos de medição */
typedef enum {
    BH1750_CONT_H_RES  = 0x10, // 1 lx resolução, 120 ms
    BH1750_CONT_H2_RES = 0x11, // 0.5 lx resolução
    BH1750_CONT_L_RES  = 0x13, // 4 lx resolução
    BH1750_ONE_H_RES   = 0x20,
    BH1750_ONE_H2_RES  = 0x21,
    BH1750_ONE_L_RES   = 0x23
} bh1750_mode_t;

/* Contexto do sensor */
typedef struct {
    uint8_t i2c_addr;
    bh1750_mode_t mode;
    uint32_t lux_x100;   // Lux * 100 (sem float)
} bh1750_ctx_t;

/* API */
bool bh1750_init(bh1750_ctx_t *ctx, uint8_t addr, bh1750_mode_t mode);
bool bh1750_read(bh1750_ctx_t *ctx);

#endif // BH1750_H
