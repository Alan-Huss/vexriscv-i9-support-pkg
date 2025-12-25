// time_driver.c

#include <generated/csr.h>
#include "time_driver.h"
#include <system.h> // busy_wait_us

void delay_ms(uint32_t ms) {
    for (uint32_t i = 0; i < ms; i++) {
        timer0_update_value_write(1);
        while (timer0_value_read() != 0);
    }
}


void busy_wait_ms(unsigned int ms) {
    for (unsigned int i = 0; i < ms; ++i) {
        busy_wait_us(1000);
    }
}
