/*
 * ST7789.h - Driver C para o display ST7789
 * Portado para o SoC LiteX com base nos CSRs definidos pelo usuário.
 */

#ifndef ST7789_H
#define ST7789_H

#include <stdint.h>

// --- Comandos do ST77xx (de Adafruit_ST77xx.h) ---
// Estes são os comandos que você pode enviar com st7789_write_command()
#define ST77XX_NOP 0x00
#define ST77XX_SWRESET 0x01
#define ST77XX_RDDID 0x04
#define ST77XX_RDDST 0x09

#define ST77XX_SLPIN 0x10
#define ST77XX_SLPOUT 0x11
#define ST77XX_PTLON 0x12
#define ST77XX_NORON 0x13

#define ST77XX_INVOFF 0x20
#define ST77XX_INVON 0x21
#define ST77XX_DISPOFF 0x28
#define ST77XX_DISPON 0x29
#define ST77XX_CASET 0x2A
#define ST77XX_RASET 0x2B
#define ST77XX_RAMWR 0x2C
#define ST77XX_RAMRD 0x2E

#define ST77XX_PTLAR 0x30
#define ST77XX_TEOFF 0x34
#define ST77XX_TEON 0x35
#define ST77XX_MADCTL 0x36
#define ST77XX_COLMOD 0x3A

#define ST77XX_MADCTL_MY 0x80
#define ST77XX_MADCTL_MX 0x40
#define ST77XX_MADCTL_MV 0x20
#define ST77XX_MADCTL_ML 0x10
#define ST77XX_MADCTL_RGB 0x00

// --- Cores 16-bit (Formato 565) ---
#define ST77XX_BLACK 0x0000
#define ST77XX_WHITE 0xFFFF
#define ST77XX_RED 0xF800
#define ST77XX_GREEN 0x07E0
#define ST77XX_BLUE 0x001F
#define ST77XX_CYAN 0x07FF
#define ST77XX_MAGENTA 0xF81F
#define ST77XX_YELLOW 0xFFE0
#define ST77XX_ORANGE 0xFC00

// --- Funções Públicas do Driver ---

/**
 * @brief Inicializa o driver do display ST7789.
 * Realiza o reset por hardware e envia a sequência de comandos de inicialização.
 * @param width A largura do seu display (ex: 240).
 * @param height A altura do seu display (ex: 240, 280, 320).
 */
void st7789_init(uint16_t width, uint16_t height);

/**
 * @brief Define a rotação do display (0-3).
 * @param m 0 = 0°, 1 = 90°, 2 = 180°, 3 = 270°.
 */
void st7789_set_rotation(uint8_t m);

/**
 * @brief Define a "janela" de memória para onde os pixels subsequentes serão escritos.
 * @param x Coordenada X inicial.
 * @param y Coordenada Y inicial.
 * @param w Largura da janela.
 * @param h Altura da janela.
 */
void st7789_set_addr_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h);

/**
 * @brief Envia um byte de COMANDO para o display.
 * @param cmd O byte de comando (ex: ST77XX_SWRESET).
 */
void st7789_write_command(uint8_t cmd);

/**
 * @brief Envia um byte de DADO para o display.
 * @param data O byte de dado.
 */
void st7789_write_data(uint8_t data);

/**
 * @brief Envia múltiplos bytes de DADO para o display.
 * @param data Ponteiro para o buffer de dados.
 * @param len Número de bytes a enviar.
 */
void st7789_write_data_buffer(const uint8_t* data, int len);

/**
 * @brief Preenche um retângulo no display com uma cor sólida.
 * @param x Coordenada X inicial.
 * @param y Coordenada Y inicial.
 * @param w Largura do retângulo.
 * @param h Altura do retângulo.
 * @param color Cor 16-bit (565).
 */
void st7789_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
 * @brief Controla o pino de Backlight (BLK).
 * @param val 1 para ligar, 0 para desligar.
 */
void st7789_set_backlight(int val);


// --- Funções de Baixo Nível (usadas por st7789_fill_rect para performance) ---

/**
 * @brief Define o pino Data/Command (DC).
 * @param val 0 para Comando, 1 para Dado.
 */
void st7789_dc_set(int val);

void st7789_draw_pixel(uint16_t x, uint16_t y, uint16_t color);

/**
 * @brief Define o pino Chip Select (CS) do SPI.
 * @param val 1 para selecionar (ativo), 0 para desselecionar.
 */
void st7789_spi_cs_set(int val);

/**
 * @brief Envia um único byte pelo SPI (bloqueante).
 * @param data O byte a ser enviado.
 */
void st7789_spi_write_byte(uint8_t data);

extern int16_t _width, _height; // Largura e altura atuais (após rotação)

#endif // ST7789_H