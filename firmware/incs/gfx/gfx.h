/*
 * gfx.h - Biblioteca gráfica (GFX) para LiteX (ATUALIZADA)
 * Fornece funções de texto e primitivas de desenho.
 */

#ifndef GFX_H
#define GFX_H

#include <stdint.h>

// --- Funções de Texto ---
void gfx_set_text_color(uint16_t color);
void gfx_set_cursor(int16_t x, int16_t y);
void gfx_set_text_size(uint8_t size);
void gfx_print(const char* str);
void gfx_draw_char(int16_t x, int16_t y, unsigned char c, uint16_t color, uint8_t size);

// --- Funções de Primitivas ---

/**
 * @brief Preenche a tela inteira com uma cor.
 */
void gfx_fill_screen(uint16_t color);

/**
 * @brief Desenha um pixel. Função base para todo desenho.
 */
void gfx_draw_pixel(int16_t x, int16_t y, uint16_t color);

/**
 * @brief Desenha uma linha vertical (otimizado).
 */
void gfx_draw_fast_vline(int16_t x, int16_t y, int16_t h, uint16_t color);

/**
 * @brief Desenha uma linha horizontal (otimizado).
 */
void gfx_draw_fast_hline(int16_t x, int16_t y, int16_t w, uint16_t color);

/**
 * @brief Desenha uma linha entre dois pontos (Algoritmo de Bresenham).
 */
void gfx_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);

/**
 * @brief Desenha um retângulo (apenas contorno).
 */
void gfx_draw_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

/**
 * @brief Desenha um retângulo preenchido (otimizado).
 */
void gfx_fill_rect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

/**
 * @brief Desenha um círculo (apenas contorno).
 */
void gfx_draw_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color);

/**
 * @brief Desenha um círculo preenchido.
 */
void gfx_fill_circle(int16_t x0, int16_t y0, int16_t r, uint16_t color);

void gfx_draw_bitmap(int16_t x, int16_t y, const uint16_t *bitmap, int16_t w, int16_t h);

#endif // GFX_H