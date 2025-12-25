/*
 * ST7789.c - Implementação do driver C para ST7789 no LiteX
 */

#include "ST7789.h"
#include <time.h>   // Para busy_wait_ms()
#include <generated/csr.h>


// --- Constantes internas ---
#define ST_CMD_DELAY 0x80 // Flag de delay na lista de comandos

// --- Variáveis de Estado (static) ---
// Armazenam os offsets e dimensões do display
int16_t _width, _height;
static int16_t _xstart, _ystart;
static uint8_t _colstart, _rowstart, _colstart2, _rowstart2;
static uint16_t windowWidth, windowHeight;

// --- Sequência de Inicialização ---
static const uint8_t generic_st7789[] = {
    9,                              // 9 comandos na lista
    ST77XX_SWRESET,   ST_CMD_DELAY, // 1: Reset por software
      1,                          // delay 150ms
    ST77XX_SLPOUT ,   ST_CMD_DELAY, // 2: Sair do modo Sleep
      1,                           // delay 10ms
    ST77XX_COLMOD , 1+ST_CMD_DELAY, // 3: Modo de cor
      0x55,                         // 16-bit (RGB565)
      1,                           // delay 10ms
    ST77XX_MADCTL , 1,              // 4: Controle de acesso à memória
      0x08,                         // Row/col addr, bottom-top refresh (padrão)
    ST77XX_CASET  , 4,              // 5: Column addr set
      0x00, 0, 0, 240,              // XSTART = 0, XEND = 240
    ST77XX_RASET  , 4,              // 6: Row addr set
      0x00, 0, 320>>8, 320&0xFF,    // YSTART = 0, YEND = 320
    ST77XX_INVON  ,   ST_CMD_DELAY, // 7: Inversão ligada
      1,
    ST77XX_NORON  ,   ST_CMD_DELAY, // 8: Modo normal
      1,                           // delay
    ST77XX_DISPON ,   ST_CMD_DELAY, // 9: Display ligado
      1                            // delay
};

// --- Funções de Abstração de Hardware (HAL) ---


void st7789_dc_set(int val) {
    lcd_dc_out_write(val);
}

static void lcd_reset_set(int val) {
    lcd_reset_out_write(val);
}

void st7789_set_backlight(int val) {
    lcd_blk_out_write(val);
}

void st7789_spi_cs_set(int val) {
    // Assumindo CS ativo em 1 (com base em CSR_SPI_CS_SEL_OFFSET = 0)
    spi_cs_write(val); 
}

void st7789_spi_write_byte(uint8_t data) {
    spi_mosi_write(data);
    // Inicia a transmissão de 8 bits
    spi_control_write((8 << CSR_SPI_CONTROL_LENGTH_OFFSET) | (1 << CSR_SPI_CONTROL_START_OFFSET));
    // Espera a transmissão terminar (poll no bit 'done')
    while (!(spi_status_read() & (1 << CSR_SPI_STATUS_DONE_OFFSET)));
}

void st7789_draw_pixel(uint16_t x, uint16_t y, uint16_t color) {
    if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height)) {
        return; // Fora da tela
    }
    
    st7789_set_addr_window(x, y, 1, 1);
    uint8_t hi = color >> 8;
    uint8_t lo = color & 0xFF;

    st7789_dc_set(1);
    st7789_spi_cs_set(1);
    st7789_spi_write_byte(hi);
    st7789_spi_write_byte(lo);
    st7789_spi_cs_set(0);
}
// --- Funções Internas do Driver ---

/**
 * @brief Executa uma lista de comandos de inicialização.
 * (Portado de Adafruit_ST77xx::displayInit)
 */
static void st7789_run_command_list(const uint8_t *addr) {
    uint8_t numCommands, cmd, numArgs;
    uint16_t ms;

    numCommands = *addr++; // Número de comandos
    while (numCommands--) {
        cmd = *addr++;     // Comando
        numArgs = *addr++; // Número de argumentos
        ms = numArgs & ST_CMD_DELAY;
        numArgs &= ~ST_CMD_DELAY;

        // Envia comando
        st7789_write_command(cmd);
        
        // Envia argumentos (dados)
        if (numArgs > 0) {
            st7789_write_data_buffer(addr, numArgs);
            addr += numArgs;
        }

        if (ms) {
            ms = *addr++;
            if (ms == 255) ms = 500;
            busy_wait_us(ms * 1000);
        }
    }
}

// --- Implementação das Funções Públicas ---

void st7789_write_command(uint8_t cmd) {
    st7789_dc_set(0); // DC baixo para comando
    st7789_spi_cs_set(1);
    st7789_spi_write_byte(cmd);
    st7789_spi_cs_set(0);
}

void st7789_write_data(uint8_t data) {
    st7789_dc_set(1); // DC alto para dado
    st7789_spi_cs_set(1);
    st7789_spi_write_byte(data);
    st7789_spi_cs_set(0);
}

void st7789_write_data_buffer(const uint8_t* data, int len) {
    st7789_dc_set(1); // DC alto para dado
    st7789_spi_cs_set(1);
    for (int i = 0; i < len; i++) {
        st7789_spi_write_byte(data[i]);
    }
    st7789_spi_cs_set(0);
}

void st7789_init(uint16_t width, uint16_t height) {
    windowWidth = width;
    windowHeight = height;

    // Lógica de offset (de Adafruit_ST7789.cpp)
    if (width == 240 && height == 240) { // Ex: 1.3", 1.54"
        _rowstart = (320 - height);
        _rowstart2 = 0;
        _colstart = _colstart2 = (240 - width);
    } else if (width == 135 && height == 240) { // Ex: 1.14"
        _rowstart = _rowstart2 = (int)((320 - height) / 2);
        _colstart = (int)((240 - width + 1) / 2);
        _colstart2 = (int)((240 - width) / 2);
    } else { // Ex: 1.47", 1.69", 2.0"
        _rowstart = _rowstart2 = (int)((320 - height) / 2);
        _colstart = _colstart2 = (int)((240 - width) / 2);
    }

    // Reset por hardware
    lcd_reset_set(0);
    busy_wait_us(50 * 1000);
    lcd_reset_set(1);
    busy_wait_us(50 * 1000);

    // Executa a lista de comandos de inicialização
    st7789_run_command_list(generic_st7789);

    // Define a rotação padrão
    st7789_set_rotation(0);

    // Liga o backlight
    st7789_set_backlight(1);
}

void st7789_set_rotation(uint8_t m) {
    uint8_t madctl = 0;
    uint8_t rotation = m & 3; // 0-3

    switch (rotation) {
    case 0:
        madctl = ST77XX_MADCTL_MX | ST77XX_MADCTL_MY | ST77XX_MADCTL_RGB;
        _xstart = _colstart;
        _ystart = _rowstart;
        _width = windowWidth;
        _height = windowHeight;
        break;
    case 1:
        madctl = ST77XX_MADCTL_MY | ST77XX_MADCTL_MV | ST77XX_MADCTL_RGB;
        _xstart = _rowstart;
        _ystart = _colstart2;
        _height = windowWidth;
        _width = windowHeight;
        break;
    case 2:
        madctl = ST77XX_MADCTL_RGB;
        _xstart = _colstart2;
        _ystart = _rowstart2;
        _width = windowWidth;
        _height = windowHeight;
        break;
    case 3:
        madctl = ST77XX_MADCTL_MX | ST77XX_MADCTL_MV | ST77XX_MADCTL_RGB;
        _xstart = _rowstart2;
        _ystart = _colstart;
        _height = windowWidth;
        _width = windowHeight;
        break;
    }

    st7789_write_command(ST77XX_MADCTL);
    st7789_write_data(madctl);
}

void st7789_set_addr_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    x += _xstart;
    y += _ystart;
    uint16_t x_end = x + w - 1;
    uint16_t y_end = y + h - 1;

    uint8_t xa_data[4] = { (x >> 8) & 0xFF, x & 0xFF, (x_end >> 8) & 0xFF, x_end & 0xFF };
    uint8_t ya_data[4] = { (y >> 8) & 0xFF, y & 0xFF, (y_end >> 8) & 0xFF, y_end & 0xFF };

    st7789_write_command(ST77XX_CASET);
    st7789_write_data_buffer(xa_data, 4);

    st7789_write_command(ST77XX_RASET);
    st7789_write_data_buffer(ya_data, 4);

    st7789_write_command(ST77XX_RAMWR);
}

void st7789_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    // 1. Define a janela de desenho
    st7789_set_addr_window(x, y, w, h);

    // 2. Prepara para enviar dados (DC alto)
    st7789_dc_set(1);
    
    // 3. Seleciona o chip
    st7789_spi_cs_set(1);

    // 4. Separa a cor em 2 bytes
    uint8_t hi = (color >> 8) & 0xFF;
    uint8_t lo = color & 0xFF;
    
    // 5. Envia os pixels (w * h) vezes
    // Usar 'long' para evitar overflow em displays grandes
    long num_pixels = (long)w * (long)h;
    for (long i = 0; i < num_pixels; i++) {
        st7789_spi_write_byte(hi);
        st7789_spi_write_byte(lo);
    }

    // 6. Desseleciona o chip
    st7789_spi_cs_set(0);
}