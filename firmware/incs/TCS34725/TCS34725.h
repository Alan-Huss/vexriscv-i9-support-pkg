#ifndef TCS34725_H
#define TCS34725_H

#include <stdint.h>
#include <stdbool.h>

/* Endereço I2C */
#define TCS34725_I2C_ADDR 0x29

/* Ganho */
typedef enum {
    TCS34725_GAIN_1X  = 0x00,
    TCS34725_GAIN_4X  = 0x01,
    TCS34725_GAIN_16X = 0x02,
    TCS34725_GAIN_60X = 0x03
} tcs34725_gain_t;

/* Tempo de integração */
typedef enum {
    TCS34725_INTEGRATION_2_4MS  = 0xFF,
    TCS34725_INTEGRATION_24MS   = 0xF6,
    TCS34725_INTEGRATION_50MS   = 0xEB,
    TCS34725_INTEGRATION_101MS  = 0xD5,
    TCS34725_INTEGRATION_154MS  = 0xC0,
    TCS34725_INTEGRATION_700MS  = 0x00
} tcs34725_integration_t;

/* Contexto */
typedef struct {
    uint8_t i2c_addr;
    tcs34725_gain_t gain;
    tcs34725_integration_t integration;
} tcs34725_ctx_t;

/* API */
bool tcs34725_init(tcs34725_ctx_t *ctx,
                   tcs34725_gain_t gain,
                   tcs34725_integration_t integration);

bool tcs34725_read_raw(tcs34725_ctx_t *ctx,
                       uint16_t *clear,
                       uint16_t *red,
                       uint16_t *green,
                       uint16_t *blue);

#endif
