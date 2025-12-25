// aht10.c
// Driver do sensor AHT10 usando bb_i2c (bit-banging LiteX)
#include <stdio.h>
#include <stdlib.h>   // abs()

#include "aht10.h"
#include "i2c_driver.h"

#include <stdint.h>
#include <stdbool.h>
#include "time_driver.h"

/* ================= Implementação ================= */

bool aht10_init(void) {
    uint8_t init_cmd[3] = {0xBE, 0x08, 0x00};

    bb_i2c_init();
    delay_ms(40);

    /* Init correto para AHT20/AHT21 */
    bb_i2c_write(AHT10_I2C_ADDR, init_cmd, 3);

    delay_ms(20);

    /* Descartar primeira leitura */
    aht10_data_t dummy;
    aht10_read(&dummy);
    delay_ms(80);

    return true;
}




bool aht10_read(aht10_data_t *data) {
    uint8_t measure_cmd[3] = {0xAC, 0x33, 0x00};
    uint8_t rx[7];

    if (!bb_i2c_write(AHT10_I2C_ADDR, measure_cmd, 3)) {
        return false;
    }

    /* Tempo real de conversão (~80 ms) */
    for (int i = 0; i < 10; i++) {
        delay_ms(10);

        if (!bb_i2c_read(AHT10_I2C_ADDR, rx, 7)) {
            return false;
        }

        if (!(rx[0] & 0x80)) {
            break;
        }
    }

    if (rx[0] & 0x80) {
        return false;
    }

    /* Extrai dados brutos (20 bits) */
    uint32_t raw_humi =
        ((uint32_t)rx[1] << 12) |
        ((uint32_t)rx[2] << 4)  |
        ((uint32_t)(rx[3] & 0xF0) >> 4);

    uint32_t raw_temp =
        ((uint32_t)(rx[3] & 0x0F) << 16) |
        ((uint32_t)rx[4] << 8) |
        rx[5];

    int32_t umidade_x100 = (int32_t)(((uint64_t)raw_humi * 10000ULL) >> 20);

    int32_t temperatura_x100 = (int32_t)(((uint64_t)raw_temp * 20000ULL) >> 20) - 5000;


    data->temperatura = temperatura_x100;
    data->umidade     = umidade_x100;

    return true;
}