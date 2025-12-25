// time_driver.h

#ifndef TIME_DRIVER_H
#define TIME_DRIVER_H

#include <stdint.h>

// --- Protótipos das Funções de Tempo (Implementadas em time_driver.c) ---

/**
 * Obtém o tempo atual do sistema em milissegundos.
 * Baseado na leitura e escala do registrador TIMER0 (CSR_TIMER0_VALUE).
 * @return Tempo decorrido em milissegundos.
 */
uint32_t time_get_ms(void);

/**
 * Causa um atraso de tempo (busy-waiting).
 * Baseado em loops de NOP ou no registrador TIMER0.
 * @param ms Milissegundos de atraso.
 */
void delay_ms(uint32_t ms);

void busy_wait_ms(unsigned int ms);

#endif // TIME_DRIVER_H