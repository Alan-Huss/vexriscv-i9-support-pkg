#ifndef BB_I2C_DRIVER_H
#define BB_I2C_DRIVER_H

#include <stdint.h>
#include <stdbool.h>

/* Inicialização do barramento I2C (bit-banging) */
void bb_i2c_init(void);

/* Operações de alto nível */
bool bb_i2c_write(uint8_t addr, const uint8_t *data, uint8_t len);
bool bb_i2c_read(uint8_t addr, uint8_t *data, uint8_t len);
int i2c_scan(uint8_t *found, uint8_t max_found);
void i2c_bus_recover(void);
int bb_i2c_bus_idle(void);


#endif // BB_I2C_DRIVER_H