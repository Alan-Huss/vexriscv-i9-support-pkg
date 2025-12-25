#ifndef AHT10_H
#define AHT10_H

#include <stdint.h>
#include <stdbool.h>

/* Endereço I2C padrão do AHT10 */
#define AHT10_I2C_ADDR 0x38

/* Estrutura de dados do sensor */
typedef struct {
    int16_t temperatura; // °C * 100
    int16_t umidade;     // %RH * 100
} aht10_data_t;

/* Inicializa o sensor */
bool aht10_init(void);

/* Dispara medição e lê os dados */
bool aht10_read(aht10_data_t *data);

#endif // AHT10_H
