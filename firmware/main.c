#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include <irq.h>
#include <uart.h>
#include <console.h>
#include <generated/csr.h>
#include <generated/soc.h>
#include <system.h>

/* Drivers e novas libs */
#include "i2c_driver.h"
#include "aht10.h"
#include "time_driver.h"
#include "bh1750.h"
#include "max3010x.h"
#include "TCS34725.h"
#include "ST7789.h"
#include "gfx.h"

static max3010x_ctx_t hr;
static bh1750_ctx_t bh1750;
static tcs34725_ctx_t color;
uint8_t sensores[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
int n_sensores = 0;


// ============================================
// === Protótipos Locais ===
// ============================================
static uint16_t rgb_to_565(uint16_t r, uint16_t g, uint16_t b);
static void draw_color_square(uint16_t color_565);
static void cabecalho_tabela(void);
void color_task(void);
// ============================================
// === Utils de cor / display ===
// ============================================

static uint16_t rgb_to_565_norm(uint16_t c,
                                uint16_t r,
                                uint16_t g,
                                uint16_t b) {

    if (c == 0) c = 1; // evita divisão por zero

    // Normaliza RGB pela luz total
    r = (uint32_t)r * 255 / c;
    g = (uint32_t)g * 255 / c;
    b = (uint32_t)b * 255 / c;

    // Limita
    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;

    return ((r & 0xF8) << 8) |
           ((g & 0xFC) << 3) |
           ( b >> 3);
}

// ============================================
// === Gamma correction (γ ≈ 2.2) ===
// ============================================
static const uint8_t gamma8_lut[256] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2,
    2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 5, 5, 5, 5,
    6, 6, 6, 6, 7, 7, 7, 8, 8, 8, 9, 9, 10, 10, 10, 11,
    11, 12, 12, 13, 13, 14, 14, 15, 15, 16, 17, 17, 18, 18, 19, 20,
    20, 21, 22, 22, 23, 24, 25, 25, 26, 27, 28, 29, 29, 30, 31, 32,
    33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48,
    49, 50, 51, 52, 54, 55, 56, 57, 58, 59, 61, 62, 63, 64, 66, 67,
    68, 70, 71, 72, 74, 75, 76, 78, 79, 81, 82, 84, 85, 87, 88, 90,
    91, 93, 94, 96, 97, 99, 101, 102, 104, 105, 107, 109, 110, 112, 114, 115,
    117, 119, 120, 122, 124, 126, 127, 129, 131, 133, 134, 136, 138, 140, 142, 143,
    145, 147, 149, 151, 152, 154, 156, 158, 160, 162, 164, 165, 167, 169, 171, 173,
    175, 177, 179, 181, 183, 185, 187, 189, 191, 193, 195, 197, 199, 201, 203, 205,
    207, 209, 211, 213, 215, 217, 219, 221, 223, 225, 227, 229, 231, 233, 235, 237,
    239, 241, 243, 245, 247, 249, 251, 253, 255
};
static uint16_t rgb_to_565_wb_gamma(uint16_t c,
                                    uint16_t r,
                                    uint16_t g,
                                    uint16_t b) {

    if (c == 0) c = 1;

    // Normalização pelo canal CLEAR (0–255)
    r = (uint32_t)r * 255 / c;
    g = (uint32_t)g * 255 / c;
    b = (uint32_t)b * 255 / c;

    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;

    // -----------------------------
    // White Balance (ganhos fixos)
    // Escala em % (100 = 1.0)
    // -----------------------------
    r = (r * 110) / 100; // +10%
    g = (g * 85)  / 100; // -15%
    b = (b * 105) / 100; // +5%

    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;

    // Gamma correction
    r = gamma8_lut[r];
    g = gamma8_lut[g];
    b = gamma8_lut[b];

    // RGB565
    return ((r & 0xF8) << 8) |
           ((g & 0xFC) << 3) |
           ( b >> 3);
}


static uint16_t rgb_to_565_gamma(uint16_t c,
                                 uint16_t r,
                                 uint16_t g,
                                 uint16_t b) {

    if (c == 0) c = 1;

    // Normaliza para 0–255 usando o canal CLEAR
    r = (uint32_t)r * 255 / c;
    g = (uint32_t)g * 255 / c;
    b = (uint32_t)b * 255 / c;

    if (r > 255) r = 255;
    if (g > 255) g = 255;
    if (b > 255) b = 255;

    // Gamma correction
    r = gamma8_lut[r];
    g = gamma8_lut[g];
    b = gamma8_lut[b];

    // RGB565
    return ((r & 0xF8) << 8) |
           ((g & 0xFC) << 3) |
           ( b >> 3);
}




static uint16_t rgb_to_565(uint16_t r, uint16_t g, uint16_t b) {

    // Converte 16 bits → 8 bits
    r >>= 8;
    g >>= 8;
    b >>= 8;

    return ((r & 0xF8) << 8) |
           ((g & 0xFC) << 3) |
           ( b >> 3);
}

static void draw_color_square(uint16_t color_565) {

    int size = 100;
    int x = (240 - size) / 2;
    int y = (320 - size) / 2;

    gfx_fill_rect(x, y, size, size, color_565);
}


// ============================================
// === Função de aplicação ===
// ============================================

static void lux(void) {

    if (!bh1750_read(&bh1750)) {
        printf("Erro leitura BH1750\n");
        return ;
    }

    //printf("Iluminancia: %lu.%02lu lx\n",
    //       bh1750.lux_x100 / 100,
    //       bh1750.lux_x100 % 100);

}


static void heart_rate(void) {
    static uint32_t t_ms = 0;

    for(int i = 0; i < 100; i++) {
        max3010x_read_fifo(&hr);
        max3010x_update(&hr, t_ms);

        //printf("IR:%lu RED:%lu BPM:%d\n",
        //       hr.ir_value,
        //       hr.red_value,
        //       hr.bpm);

        delay_ms(10);
        t_ms += 10;
    }
}

void color_task(void) {
    uint16_t c, r, g, b;

    if (!tcs34725_init(&color,
                       TCS34725_GAIN_16X,
                       TCS34725_INTEGRATION_154MS)) {
        printf("Erro TCS34725\n");
        return;
    }

    while (1) {
        if (tcs34725_read_raw(&color, &c, &r, &g, &b)) {
            printf("C:%u R:%u G:%u B:%u\n", c, r, g, b);
        }
        delay_ms(1000000);
    }
}

static bool contem_elemento(uint8_t vetor[], int tamanho, uint8_t busca) {
    for (int i = 0; i < tamanho; i++) {
        if (vetor[i] == busca) {
            return true; // Encontrou!
        }
    }
    return false; // Percorreu tudo e não achou
}

static void scan_init(void) {
    uint8_t devices[16];
    int n = i2c_scan(devices, sizeof(devices));

    if(n_sensores != n){
        n_sensores = n;
        gfx_fill_screen(ST77XX_BLACK);
        cabecalho_tabela(); 
    }

    if (n < 0) {
        printf("I2C scan failed — assuming no devices\n");
        n = 0;  // força remoção
    }

    // --- 1. REMOVER SENSORES QUE SUMIRAM ---
    for (int j = 0; j < 16; j++) {
        // Se existe um sensor registrado que NÃO apareceu no scan físico atual
        if (sensores[j] != 0 && !contem_elemento(devices, n, sensores[j])) {
            printf("Sensor removido: %02X\n", sensores[j]);
            sensores[j] = 0;
        }
    }


    // Se n=0, o laço de remoção abaixo já vai limpar o array sensores automaticamente
    printf("I2C devices found: %d\n", n);

    // --- 2. ADICIONAR E INICIALIZAR NOVOS SENSORES ---
    for (int i = 0; i < n; i++) {
        // Se o dispositivo físico NÃO está na nossa lista lógica, adicionamos
        if (!contem_elemento(sensores, 16, devices[i])) {
            
            // Procura o primeiro slot vazio (0) para inserir
            for (int k = 0; k < 16; k++) {
                if (sensores[k] == 0) {
                    sensores[k] = devices[i];
                    printf("Novo dispositivo: %02X - ", devices[i]);
                    
                    // Inicialização específica
                    switch (devices[i]) {
                        case 0x23:
                            printf("BH1750\n");
                            if (!bh1750_init(&bh1750, BH1750_ADDR_LOW, BH1750_CONT_H_RES)) printf("Erro BH1750\n");
                            break;
                        case 0x57:
                            printf("MAX3010x\n");
                            if (!max3010x_init(&hr)) printf("Erro MAX3010x\n");
                            break;
                        case 0x29:
                            printf("TCS34725\n");
                            if (!tcs34725_init(&color, TCS34725_GAIN_16X, TCS34725_INTEGRATION_154MS)) printf("Erro TCS34725\n");
                            break;
                        case 0x38:
                            printf("AHT10\n");
                            if (!aht10_init()) printf("Erro AHT10\n");
                            break;
                        default:
                            printf("Desconhecido\n");
                            break;
                    }
                    break; // Sai do laço k, já ocupou o slot
                }
            }
        }
    }
    
}

static void cabecalho_tabela(void){

    int pos_y = 0;

    gfx_fill_rect(0, 0, 320, 30, ST77XX_BLUE);
    gfx_draw_rect(0, 0, 320, 240, ST77XX_WHITE);
    gfx_draw_line(110, 0, 110, 240, ST77XX_WHITE);

    pos_y += 8;
    gfx_set_text_size(2);
    gfx_set_cursor(18, pos_y);
    gfx_print("Sensor");
    gfx_set_cursor(170, pos_y);
    gfx_print("Dados");
    pos_y += 22;
    gfx_draw_fast_hline(0, pos_y, 320, ST77XX_WHITE);
}

static void imprime_tabela(void){

    int pos_y = 8 + 22;
    char buf[16];

    if(contem_elemento(sensores, 16, 0x38) && false) {
            
        }

    // sensor BH1750 (luminosidade)
    if(contem_elemento(sensores, 16, 0x23)) {
        pos_y += 8;
        gfx_set_cursor(18, pos_y);
        gfx_print("BH1750");
        gfx_set_cursor(116,pos_y);

       // ativa a cor do background para apagar o texto anterior
        gfx_set_text_color(ST77XX_BLACK);
        snprintf(buf, sizeof(buf), "%lu", bh1750.lux_x100 / 100);
        gfx_print(buf);
        gfx_print(".");
        snprintf(buf, sizeof(buf), "%02lu", bh1750.lux_x100 % 100);
        gfx_print(buf);
        gfx_print(" Lux");

        // realiza uma nova leitura
        lux();
        // volta a cor do texto para branco
        gfx_set_text_color(ST77XX_WHITE);
        gfx_set_cursor(116,pos_y);
        snprintf(buf, sizeof(buf), "%lu", bh1750.lux_x100 / 100);
        gfx_print(buf);
        gfx_print(".");
        snprintf(buf, sizeof(buf), "%02lu", bh1750.lux_x100 % 100);
        gfx_print(buf);
        gfx_print(" Lux");
        pos_y += 22;
        gfx_draw_fast_hline(0, pos_y, 320, ST77XX_WHITE);
        printf("Iluminancia: %lu.%02lu lx\n", bh1750.lux_x100 / 100, bh1750.lux_x100 % 100);
    }
    if (contem_elemento(sensores, 16, 0x57))
    {
        gfx_draw_line(180, pos_y, 180, pos_y + 8 + 22 * 2, ST77XX_WHITE);
        gfx_draw_line(180 + 70, pos_y, 180 + 70, pos_y + 8 + 22 * 2, ST77XX_WHITE);
        pos_y += 8;
        gfx_set_cursor(116 + 12, pos_y);
        gfx_print("BPM");
        gfx_set_cursor(116 + 12 + 6 + 70, pos_y);
        gfx_print("IR");
        gfx_set_cursor(116 + 12 + 140, pos_y);
        gfx_print("RED");
        pos_y += 12;
        gfx_set_cursor(8, pos_y);
        gfx_print("MAX3010x");
        pos_y += 8;
        gfx_draw_line(110, pos_y, 320, pos_y, ST77XX_WHITE);
        pos_y += 4;
        // apaga os valores antigos
        gfx_set_text_color(ST77XX_BLACK);
        gfx_set_cursor(116 + 12, pos_y);
        snprintf(buf, sizeof(buf), "%d", hr.bpm);
        gfx_print(buf);
        gfx_set_cursor(116 + 70, pos_y);
        snprintf(buf, sizeof(buf), "%lu", hr.ir_value >> 10);
        gfx_print(buf);
        gfx_set_cursor(116 + 140, pos_y);
        snprintf(buf, sizeof(buf), "%lu", hr.red_value >> 10);
        gfx_print(buf);
        // volta a cor do texto para branco
        gfx_set_text_color(ST77XX_WHITE);
        heart_rate();
        gfx_set_cursor(116 + 12, pos_y);
        snprintf(buf, sizeof(buf), "%d", hr.bpm);
        gfx_print(buf);
        gfx_set_cursor(116 + 70, pos_y);
        snprintf(buf, sizeof(buf), "%lu", hr.ir_value >> 10);
        gfx_print(buf);
        gfx_set_cursor(116 + 140, pos_y);
        snprintf(buf, sizeof(buf), "%lu", hr.red_value >> 10);
        gfx_print(buf);
        pos_y += 22;
        gfx_draw_fast_hline(0, pos_y, 320, ST77XX_WHITE);

        printf("IR:%lu RED:%lu BPM:%d\n", hr.ir_value, hr.red_value, hr.bpm);
    }
    if (contem_elemento(sensores, 16, 0x29))
    {
        uint16_t c, r, g, b;
        uint16_t color565;

        pos_y += 8;
        gfx_set_text_color(ST77XX_BLACK);
        gfx_set_cursor(116, pos_y);
        sprintf(buf, "R:%u", r >> 8);
        gfx_print(buf);
        pos_y += 24;
        gfx_set_cursor(116, pos_y);
        sprintf(buf, "G:%u", g >> 8);
        gfx_print(buf);
        pos_y += 24;
        gfx_set_cursor(116, pos_y);
        sprintf(buf, "B:%u", b >> 8);
        gfx_print(buf);

        if (tcs34725_read_raw(&color, &c, &r, &g, &b))
        {
            // color565 = rgb_to_565(r, g, b);

            color565 = rgb_to_565_norm(c, r, g, b);

            // color565 = rgb_to_565_gamma(c, r, g, b);

            // color565 = rgb_to_565_wb_gamma(c, r, g, b);

            pos_y -= 48;
            gfx_set_text_color(ST77XX_WHITE);
            gfx_fill_rect(185, pos_y, 125, 24 * 3 - 11, color565);
            gfx_set_cursor(116, pos_y);
            sprintf(buf, "R:%u", r >> 8);
            gfx_print(buf);
            pos_y += 24;
            gfx_set_cursor(8, pos_y);
            gfx_print("TCS34725");
            gfx_set_cursor(116, pos_y);
            sprintf(buf, "G:%u", g >> 8);
            gfx_print(buf);
            pos_y = pos_y + 24;
            gfx_set_cursor(116, pos_y);
            sprintf(buf, "B:%u", b >> 8);
            gfx_print(buf);
            pos_y += 22;
            gfx_draw_fast_hline(0, pos_y, 320, ST77XX_WHITE);

            printf("R:%u G:%u B:%u RGB565:0x%04X\n", r, g, b, color565);
        }
    }
}

// ============================================
// === main ===
// ============================================

int main(void) {

#ifdef CONFIG_CPU_HAS_INTERRUPT
    irq_setmask(0);
    irq_setie(1);
#endif

    uart_init();

    bb_i2c_init();

    // Display
    st7789_init(240, 320);
    st7789_set_rotation(1);
    gfx_fill_screen(ST77XX_BLACK);

    gfx_set_text_size(2);
    gfx_set_text_color(ST77XX_WHITE);

#ifdef CSR_TIMER0_BASE
    timer0_en_write(0);
    timer0_reload_write(0);
    timer0_load_write(CONFIG_CLOCK_FREQUENCY / 1000000);
    timer0_en_write(1);
    timer0_update_value_write(1);
#endif


    while (1) {
        scan_init();
        imprime_tabela();
    delay_ms(100000);
}

    return 0;
}
