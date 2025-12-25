// i2c_driver.c
// Driver I2C por bit-banging usando CSR (LiteX)

#include "i2c_driver.h"

#include <generated/csr.h>
#include <system.h>   // busy_wait_us
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>


/* =========================================================
 * Internos do driver (NÃO exportados)
 * ========================================================= */

static uint32_t i2c_w_reg = 0;

/* Delay básico do barramento */
static void i2c_delay(void) {
    busy_wait_us(5);
}

/* Controle das linhas */
static void i2c_set_scl(int val) {
    if (val) i2c_w_reg |=  (1 << CSR_I2C_W_SCL_OFFSET);
    else     i2c_w_reg &= ~(1 << CSR_I2C_W_SCL_OFFSET);
    i2c_w_write(i2c_w_reg);
}

static void i2c_set_sda(int val) {
    if (val) i2c_w_reg |=  (1 << CSR_I2C_W_SDA_OFFSET);
    else     i2c_w_reg &= ~(1 << CSR_I2C_W_SDA_OFFSET);
    i2c_w_write(i2c_w_reg);
}

static void i2c_set_oe(int val) {
    if (val) i2c_w_reg |=  (1 << CSR_I2C_W_OE_OFFSET);
    else     i2c_w_reg &= ~(1 << CSR_I2C_W_OE_OFFSET);
    i2c_w_write(i2c_w_reg);
}

static int i2c_read_sda(void) {
    return (i2c_r_read() >> CSR_I2C_R_SDA_OFFSET) & 0x1;
}

/* Condições I2C */
static void i2c_start(void) {
    i2c_set_sda(1);
    i2c_set_scl(1);
    i2c_delay();
    i2c_set_sda(0);
    i2c_delay();
    i2c_set_scl(0);
}

static void i2c_stop(void) {
    i2c_set_sda(0);
    i2c_set_scl(1);
    i2c_delay();
    i2c_set_sda(1);
    i2c_delay();
}

/* Byte-level */
static bool i2c_write_byte(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        i2c_set_sda((data & 0x80) != 0);
        i2c_delay();
        i2c_set_scl(1);
        i2c_delay();
        i2c_set_scl(0);
        data <<= 1;
    }

    /* ACK */
    i2c_set_oe(0);
    i2c_delay();
    i2c_set_scl(1);
    i2c_delay();
    bool ack = !i2c_read_sda();
    i2c_set_scl(0);
    i2c_set_oe(1);

    return ack;
}

static uint8_t i2c_read_byte(bool ack) {
    uint8_t data = 0;

    i2c_set_oe(0);

    for (int i = 0; i < 8; i++) {
        i2c_set_scl(1);
        i2c_delay();
        data = (data << 1) | i2c_read_sda();
        i2c_set_scl(0);
        i2c_delay();
    }

    /* ACK / NACK */
    i2c_set_oe(1);
    i2c_set_sda(!ack);
    i2c_delay();
    i2c_set_scl(1);
    i2c_delay();
    i2c_set_scl(0);
    i2c_set_sda(1);

    return data;
}

int bb_i2c_bus_idle(void) {
    /* solta o barramento */
    i2c_set_oe(0);
    i2c_delay();

    int sda = i2c_read_sda();

    /* SCL: assume alto se o bit W está setado */
    int scl = (i2c_w_reg >> CSR_I2C_W_SCL_OFFSET) & 1;

    /* reassume controle */
    i2c_set_oe(1);

    return sda && scl;
}


/* =========================================================
 * API pública (bb_i2c_*)
 * ========================================================= */

void bb_i2c_init(void) {
    i2c_set_oe(1);
    i2c_set_sda(1);
    i2c_set_scl(1);
    i2c_delay();
}

bool bb_i2c_write(uint8_t addr, const uint8_t *data, uint8_t len) {
    i2c_start();

    if (!i2c_write_byte(addr << 1)) {
        i2c_stop();
        return false;
    }

    for (uint8_t i = 0; i < len; i++) {
        if (!i2c_write_byte(data[i])) {
            i2c_stop();
            return false;
        }
    }

    i2c_stop();
    return true;
}

bool bb_i2c_read(uint8_t addr, uint8_t *data, uint8_t len) {
    i2c_start();

    if (!i2c_write_byte((addr << 1) | 1)) {
        i2c_stop();
        return false;
    }

    for (uint8_t i = 0; i < len; i++) {
        data[i] = i2c_read_byte(i < (len - 1));
    }

    i2c_stop();
    return true;
}
int i2c_scan(uint8_t *found, uint8_t max_found) {
    uint8_t count = 0;

    /* 1) Verifica se o barramento está em idle */
    if (!bb_i2c_bus_idle()) {
        printf(" ! I2C bus not idle (floating or stuck)\n");
        return -1;
    }

    /* 2) Scan */
    for (uint8_t addr = 1; addr < 127; addr++) {

        if (count >= max_found) {
            break;  // evita overflow do vetor
        }

        /* Garante barramento limpo */
        bb_i2c_init();
        busy_wait_us(50);

        /* Probe (escrita sem dados) */
        if (bb_i2c_write(addr, NULL, 0)) {

            /* Confirma ACK para evitar falso positivo */
            busy_wait_us(10);
            if (bb_i2c_write(addr, NULL, 0)) {
                found[count++] = addr;
            }
        }
    }

    return count;
}

void i2c_bus_recover(void) {
    i2c_set_oe(0); // solta SDA

    for (int i = 0; i < 9; i++) {
        i2c_set_scl(0);
        busy_wait_us(5);
        i2c_set_scl(1);
        busy_wait_us(5);
    }

    // STOP
    i2c_set_sda(0);
    busy_wait_us(5);
    i2c_set_scl(1);
    busy_wait_us(5);
    i2c_set_sda(1);
}