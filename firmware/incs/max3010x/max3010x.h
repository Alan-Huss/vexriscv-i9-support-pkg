#ifndef MAX3010X_H
#define MAX3010X_H

#include <stdint.h>
#include <stdbool.h>

#define MAX3010X_I2C_ADDR 0x57

#define IR_BUF     8
#define RR_BUF     5

typedef struct {
    uint8_t  i2c_addr;

    uint32_t ir_value;
    uint32_t red_value;

    bool     finger_detected;
    int      bpm;

    /* --- processamento interno --- */
    uint32_t ir_buf[IR_BUF];
    uint8_t  ir_idx;

    int32_t  ir_dc;
    bool     peak_high;

    uint32_t rr[RR_BUF];
    uint8_t  rr_i;

    uint32_t last_peak_ms;

} max3010x_ctx_t;

/* API */
bool max3010x_init(max3010x_ctx_t *ctx);
bool max3010x_read_fifo(max3010x_ctx_t *ctx);
void max3010x_update(max3010x_ctx_t *ctx, uint32_t elapsed_ms);

#endif
