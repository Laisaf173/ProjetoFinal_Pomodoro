#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "pico/bootrom.h"
#include "ProjetoFinal_Pomodoro.pio.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"    // Para o joystick
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "hardware/pwm.h"


#define NUM_PIXELS 25
#define OUT_PIN 7
const uint button_0 = 5;

// Definições I2C
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define OLED_ADDRESS 0x3C
#define OLED_WIDTH 128
#define OLED_HEIGHT 64

// Definições para o joystick
#define JOYSTICK_X_PIN 26  // GPIO para eixo X
#define JOYSTICK_Y_PIN 27  // GPIO para eixo Y
#define JOYSTICK_BTN 22    // GPIO para botão do joystick

// Definições para o buzzer
#define BUZZER_PIN 10
#define BUZZER_PWM_CHAN PWM_CHAN_B  // Canal B para pino 10
#define BUZZER_DURATION 2000        // Duração do som do buzzer (em ms)

// Definições para o Pomodoro
#define POMODORO_WORK_TIME    (25 * 60)  // 25 minutos (em segundos)
#define POMODORO_SHORT_BREAK  (5 * 60)   // 5 minutos (em segundos)
#define POMODORO_LONG_BREAK   (15 * 60)  // 15 minutos (em segundos)

// Definições para o menu
typedef enum {
    MENU_STATE_MAIN,
    MENU_STATE_WORK_TIME,
    MENU_STATE_SHORT_BREAK,
    MENU_STATE_LONG_BREAK,
    MENU_STATE_START
} menu_state_t;

typedef enum {
    POMODORO_STATE_WORK,
    POMODORO_STATE_SHORT_BREAK,
    POMODORO_STATE_LONG_BREAK,
    POMODORO_STATE_IDLE
} pomodoro_state_t;

typedef struct {
    pomodoro_state_t state;
    uint32_t remaining_time;  // em segundos
    uint32_t last_update;     
    uint8_t completed_cycles;
} pomodoro_t;

pomodoro_t pomodoro = {
    .state = POMODORO_STATE_IDLE,
    .remaining_time = POMODORO_WORK_TIME,
    .last_update = 0,
    .completed_cycles = 0
};

// Variáveis para o menu
menu_state_t menu_state = MENU_STATE_MAIN;
int menu_selection = 0;
bool menu_active = true;

// Tempos configuráveis pelo usuário (padrões iniciais)
uint32_t user_work_time = POMODORO_WORK_TIME;
uint32_t user_short_break = POMODORO_SHORT_BREAK;
uint32_t user_long_break = POMODORO_LONG_BREAK;

// Variáveis globais para o buzzer
uint buzzer_slice_num;
bool buzzer_active = false;
uint32_t buzzer_start_time = 0;


// Configurações da matriz LED RGB que formam o tomate
double desenho_verde[25] = {
    0.0, 0.0, 0.0, 1.0, 0.0,
    0.0, 0.0, 1.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0
};

double desenho_vermelho[25] = {
    0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0,
    0.0, 1.0, 1.0, 1.0, 0.0,
    0.0, 1.0, 1.0, 1.0, 0.0,
    0.0, 0.0, 0.0, 0.0, 0.0
};

double desenho_branco[25] = {
    0.2, 0.2, 0.2, 0.0, 0.2,
    0.2, 0.2, 0.0, 0.2, 0.2,
    0.2, 0.0, 0.0, 0.0, 0.2,
    0.2, 0.0, 0.0, 0.0, 0.2,
    0.2, 0.2, 0.2, 0.2, 0.2
};

int mapa_leds[25] = {
    24, 23, 22, 21, 20,
    15, 16, 17, 18, 19,
    14, 13, 12, 11, 10,
    5,  6,  7,  8,  9,
    4,  3,  2,  1,  0
};

// Bitmap do tomate
static const uint8_t tomate_bitmap[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0x7f, 0x7f, 0x7f, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0x1f, 0x07, 0x03, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x03, 0x03, 0x0f, 0x3f, 0x3f, 0x1f, 
    0x1f, 0x1f, 0x09, 0x01, 0x03, 0x03, 0x03, 0x07, 0x03, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x01, 
    0x01, 0x0d, 0x0f, 0x0f, 0x0f, 0x0f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x40, 0x20, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 
    0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0xc0, 0xc0, 0x80, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x07, 0x1f, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x40, 0xfb, 
    0xf7, 0xe0, 0xc0, 0xb0, 0xb0, 0xb0, 0xb0, 0xb0, 0xd0, 0xe0, 0xf0, 0xf7, 0xf7, 0xf7, 0xf0, 0x10, 
    0x00, 0x00, 0x08, 0x18, 0x78, 0xf8, 0x78, 0x3c, 0x80, 0xf0, 0xf0, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xf8, 0xe0, 0xe0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 
    0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 
    0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 
    0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 
    0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 
    0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xc0, 0xe0, 0xf0, 0xfc, 0xfc, 0xf8, 
    0xf3, 0x87, 0x8f, 0x8f, 0x9f, 0x9f, 0xdf, 0xdf, 0xdf, 0xdf, 0xdf, 0xdf, 0xdf, 0x9f, 0x1f, 0x1f, 
    0x1f, 0x8c, 0xec, 0xe4, 0xf3, 0xf8, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

// Função de inicialização do display OLED
void oled_init() {
    i2c_init(I2C_PORT, 400000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    uint8_t init_sequence[] = {
        0xAE, 0xD5, 0x80, 0xA8, 0x3F, 0xD3, 0x00, 0x40,
        0x8D, 0x14, 0x20, 0x00, 0xA1, 0xC8, 0xDA, 0x12,
        0x81, 0xFF, 0xD9, 0xF1, 0xDB, 0x40, 0xA4, 0xA6, 0xAF
    };

    for (int i = 0; i < sizeof(init_sequence); i++) {
        uint8_t cmd[] = {0x00, init_sequence[i]};
        i2c_write_blocking(I2C_PORT, OLED_ADDRESS, cmd, 2, false);
    }

    uint8_t clear_display[1025] = {0x40};
    i2c_write_blocking(I2C_PORT, OLED_ADDRESS, clear_display, 1025, false);
}

// Exibe o bitmap do tomate no OLED
void display_bitmap() {
    uint8_t cmd[] = {0x00, 0x21, 0x00, 0x7F, 0x22, 0x00, 0x07};
    i2c_write_blocking(I2C_PORT, OLED_ADDRESS, cmd, sizeof(cmd), false);

    uint8_t data[1025];
    data[0] = 0x40;
    for (int i = 0; i < 1024; i++) {
        data[i + 1] = tomate_bitmap[i];
    }
    i2c_write_blocking(I2C_PORT, OLED_ADDRESS, data, 1025, false);
}

// Converte valores RGB para o formato usado pela matriz LED
uint32_t matrix_rgb(double b, double r, double g) {
    unsigned char R = r * 255;
    unsigned char G = g * 255;
    unsigned char B = b * 255;
    return (G << 24) | (R << 16) | (B << 8);
}

double desenho_atual[25]; // Para armazenar o estado atual dos LEDs
int leds_acesos;         // Contador de quantos LEDs ainda estão acesos

// Função para inicializar o desenho completo
void inicializar_desenho() {
    // Copia os LEDs verdes e vermelhos para o desenho atual
    for (int i = 0; i < 25; i++) {
        desenho_atual[i] = (desenho_verde[i] > 0.0) ? desenho_verde[i] : 
                           (desenho_vermelho[i] > 0.0) ? desenho_vermelho[i] : 0.0;
    }
    
    // Conta quantos LEDs estão acesos
    leds_acesos = 0;
    for (int i = 0; i < 25; i++) {
        if (desenho_atual[i] > 0.0) leds_acesos++;
    }
}

// Função para apagar um LED com base no tempo restante 
void atualizar_desenho_led(uint32_t tempo_total, uint32_t tempo_restante) {
    if (leds_acesos <= 0) return; // Se não há LEDs acesos, não faz nada
    
    // Calcula quantos LEDs devem estar acesos proporcionalmente ao tempo restante
    int total_leds = 0;
    for (int i = 0; i < 25; i++) {
        if (desenho_verde[i] > 0.0 || desenho_vermelho[i] > 0.0) {
            total_leds++;
        }
    }
    
    // Calcula quantos LEDs devem estar acesos com base na proporção exata de tempo restante
    float proporcao = (float)tempo_restante / tempo_total;
    int leds_para_manter = (int)(proporcao * total_leds);
    
    // Se o tempo restante não é zero, garante que pelo menos um LED fique aceso
    if (tempo_restante > 0 && leds_para_manter < 1)
        leds_para_manter = 1;
    
    // Se o tempo é zero, apaga todos os LEDs
    if (tempo_restante == 0)
        leds_para_manter = 0;
        
    // Se a quantidade de LEDs acesos for maior que a necessária, apaga os LEDs extras
    if (leds_acesos > leds_para_manter) {
        // Resetamos todos os LEDs e reacendemos a quantidade correta
        for (int i = 0; i < 25; i++) {
            desenho_atual[i] = 0.0;
        }
        
        // Reacende os LEDs na sequência, começando pelos primeiros (ordem definida)
        int leds_reacendidos = 0;
        
        // Reacende os LEDs na ordem: verde primeiro, depois vermelho
        // Primeiro os verdes
        for (int i = 24; i >= 0 && leds_reacendidos < leds_para_manter; i--) {
            if (desenho_vermelho[i] > 0.0) {
                desenho_atual[i] = desenho_vermelho[i];
                leds_reacendidos++;
            }
        }
        
        // Depois os vermelhos
        for (int i = 24; i >= 0 && leds_reacendidos < leds_para_manter; i--) {
            if (desenho_verde[i] > 0.0 && desenho_atual[i] == 0.0) {
                desenho_atual[i] = desenho_verde[i];
                leds_reacendidos++;
            }
        }
        
        leds_acesos = leds_reacendidos;
    }
    // Se não houver alteração necessária, mantém os LEDs como estão
}

uint32_t get_color(int i) {
    if (desenho_atual[i] > 0.0 && desenho_verde[i] > 0.0) return matrix_rgb(0.0, 0.0, 0.3);
    if (desenho_atual[i] > 0.0 && desenho_vermelho[i] > 0.0) return matrix_rgb(0.0, 0.3, 0.0);
    if (desenho_branco[i] == 0.2) return matrix_rgb(0.1, 0.1, 0.1);
    return matrix_rgb(0.0, 0.0, 0.0);
}


// Reinicialização do buzzer para garantir que funcione com buzzer passivo
void buzzer_init() {
    printf("Inicializando buzzer passivo no pino %d...\n", BUZZER_PIN);
    
    // Configuração direta do pino como saída GPIO primeiro
    gpio_init(BUZZER_PIN);
    gpio_set_dir(BUZZER_PIN, GPIO_OUT);
    
    // Enviar alguns pulsos diretamente pelo GPIO para testar a conexão
    for (int i = 0; i < 50; i++) {
        gpio_put(BUZZER_PIN, 1);
        sleep_us(500);  // 500us ligado
        gpio_put(BUZZER_PIN, 0);
        sleep_us(500);  // 500us desligado
    }
    
    // Aguardar antes de configurar como PWM
    sleep_ms(500);
    
    // Configurar como PWM
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM);
    
    // Obter o número do slice e do canal para o pino do buzzer
    buzzer_slice_num = pwm_gpio_to_slice_num(BUZZER_PIN);
    uint channel = pwm_gpio_to_channel(BUZZER_PIN);
    
    printf("Buzzer usando slice %d, canal %d\n", buzzer_slice_num, channel);
 
    uint32_t freq = 1000;  // Frequência inicial de 1kHz
    uint32_t clock = 125000000;
    float divider = clock / 1000000.0f;  // Dividir para 1MHz
    
    pwm_config config = pwm_get_default_config();
    pwm_config_set_clkdiv(&config, divider);
    pwm_config_set_wrap(&config, 1000000 / freq - 1);  
    
    pwm_init(buzzer_slice_num, &config, true);
    pwm_set_gpio_level(BUZZER_PIN, 500000 / freq); 
    
    
    sleep_ms(300);
    
    // Desligue o som
    pwm_set_gpio_level(BUZZER_PIN, 0);
}

// Função para tocar o buzzer com uma frequência específica
void buzzer_play(uint32_t freq) {
    if (freq == 0) {
        // Silenciar
        pwm_set_gpio_level(BUZZER_PIN, 0);
        return;
    }
    
    // Limitar a frequência entre 100 e 5000Hz
    if (freq < 100) freq = 100;
    if (freq > 5000) freq = 5000;
    
    // Cálculo simplificado para PWM
    uint32_t wrap_value = 125000000 / 128 / freq - 1;
    
    pwm_set_wrap(buzzer_slice_num, wrap_value);
    pwm_set_gpio_level(BUZZER_PIN, wrap_value / 2); // 50% de ciclo de trabalho
    
    printf("Tocando buzzer: freq=%dHz, wrap=%d\n", freq, wrap_value);
}

// Inicia o alarme do buzzer para o Pomodoro
void buzzer_start_alarm() {
    printf("Iniciando alarme do buzzer\n");
    buzzer_active = true;
    buzzer_start_time = to_ms_since_boot(get_absolute_time());
    buzzer_play(2000); // Frequência inicial
}

// Atualiza o alarme do buzzer
void buzzer_update() {
    if (!buzzer_active) return;
    
    uint32_t current_time = to_ms_since_boot(get_absolute_time());
    uint32_t elapsed = current_time - buzzer_start_time;
    
    if (elapsed >= BUZZER_DURATION) {
        // Desligar o buzzer após a duração definida
        buzzer_play(0);
        buzzer_active = false;
        printf("Alarme do buzzer encerrado\n");
        return;
    }
    
    // Alterna entre quatro frequências diferentes a cada 70ms
    uint32_t pattern_step = (elapsed / 70) % 4;
    
    switch (pattern_step) {
        case 0: buzzer_play(1500); break;
        case 1: buzzer_play(2000); break;
        case 2: buzzer_play(1800); break;
        case 3: buzzer_play(1200); break;
    }
}

// Funções do Pomodoro
void pomodoro_start(pomodoro_state_t state) {
    pomodoro.state = state;
    switch (state) {
        case POMODORO_STATE_WORK:
            pomodoro.remaining_time = user_work_time;
            break;
        case POMODORO_STATE_SHORT_BREAK:
            pomodoro.remaining_time = user_short_break;
            break;
        case POMODORO_STATE_LONG_BREAK:
            pomodoro.remaining_time = user_long_break;
            break;
        case POMODORO_STATE_IDLE:
            break;
    }
    pomodoro.last_update = time_us_64() / 1000000; // em segundos

    // Inicializa o desenho completo quando um novo ciclo começar
    inicializar_desenho();
}

// Atualiza o Pomodoro
void pomodoro_update() {
    if (pomodoro.state == POMODORO_STATE_IDLE) return;

    uint32_t current_time = time_us_64() / 1000000;
    uint32_t elapsed = current_time - pomodoro.last_update;

    if (elapsed >= 1) {
        pomodoro.last_update = current_time;
        if (pomodoro.remaining_time > 0){
            pomodoro.remaining_time -= elapsed;

            
            // Atualiza o desenho LED com base no tempo restante
            uint32_t tempo_total;
            switch (pomodoro.state) {
                case POMODORO_STATE_WORK:
                    tempo_total = user_work_time;
                    break;
                case POMODORO_STATE_SHORT_BREAK:
                    tempo_total = user_short_break;
                    break;
                case POMODORO_STATE_LONG_BREAK:
                    tempo_total = user_long_break;
                    break;
                default:
                    tempo_total = 1; 
            }
            
            atualizar_desenho_led(tempo_total, pomodoro.remaining_time);

        }
        if (pomodoro.remaining_time <= 0) {
            // Tocar o alarme quando o tempo acabar
            buzzer_start_alarm();

            if (pomodoro.state == POMODORO_STATE_WORK) {
                pomodoro.completed_cycles++;
                if (pomodoro.completed_cycles % 4 == 0)
                    pomodoro_start(POMODORO_STATE_LONG_BREAK);
                else
                    pomodoro_start(POMODORO_STATE_SHORT_BREAK);
            } else {
                pomodoro_start(POMODORO_STATE_WORK);
            }
        }
    }
}

// Funções para desenhar dígitos grandes no  display OLED
void draw_large_digit(uint8_t digit, int x, int y) {
    const uint8_t digit_width = 16;
    const uint8_t digit_height = 24;
    uint8_t cmd[] = {0x00, 0x21, x, x + digit_width - 1, 0x22, y / 8, y / 8 + (digit_height / 8) - 1};
    i2c_write_blocking(I2C_PORT, OLED_ADDRESS, cmd, sizeof(cmd), false);

    uint8_t data[1 + digit_width * (digit_height / 8)];
    data[0] = 0x40;
     const uint8_t digit_patterns[10][3][16] = {
        { // 0
            {0x00, 0x00, 0xF0, 0xF8, 0x1C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1C, 0xF8, 0xF0, 0x00, 0x00, 0x00},
            {0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00},
            {0x00, 0x00, 0x0F, 0x1F, 0x38, 0x30, 0x30, 0x30, 0x30, 0x30, 0x38, 0x1F, 0x0F, 0x00, 0x00, 0x00}
        },
        { // 1
            {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x1C, 0xFC, 0xFC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
        },
        { // 2
            {0x00, 0x00, 0x70, 0x78, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1C, 0xF8, 0xF0, 0x00, 0x00, 0x00, 0x00},
            {0x00, 0x00, 0x00, 0x00, 0x80, 0xC0, 0xE0, 0x70, 0x38, 0x1C, 0x0F, 0x07, 0x00, 0x00, 0x00, 0x00},
            {0x00, 0x00, 0x3F, 0x3F, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x38, 0x38, 0x00, 0x00, 0x00, 0x00}
        },
        { // 3
            {0x00, 0x00, 0x70, 0x78, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1C, 0xF8, 0xF0, 0x00, 0x00, 0x00, 0x00},
            {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x18, 0x18, 0x3C, 0xFF, 0xE7, 0x00, 0x00, 0x00, 0x00},
            {0x00, 0x00, 0x1C, 0x1E, 0x30, 0x30, 0x30, 0x30, 0x30, 0x38, 0x1F, 0x0F, 0x00, 0x00, 0x00, 0x00}
        },
        { // 4
            {0x00, 0x00, 0x80, 0xC0, 0xE0, 0x70, 0x38, 0x1C, 0x0E, 0x07, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00},
            {0x00, 0x00, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0xFF, 0xFF, 0x07, 0x07, 0x00, 0x00},
            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00}
        },
        { // 5
            {0x00, 0x00, 0xFC, 0xFC, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x00, 0x00, 0x00, 0x00},
            {0x00, 0x00, 0x0F, 0x0F, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1C, 0xF8, 0xF0, 0x00, 0x00, 0x00, 0x00},
            {0x00, 0x00, 0x1C, 0x1E, 0x30, 0x30, 0x30, 0x30, 0x30, 0x38, 0x1F, 0x0F, 0x00, 0x00, 0x00, 0x00}
        },
        { // 6
            {0x00, 0x00, 0xF0, 0xF8, 0x1C, 0x0E, 0x06, 0x06, 0x06, 0x06, 0x0C, 0x0C, 0x00, 0x00, 0x00, 0x00},
            {0x00, 0x00, 0xFF, 0xFF, 0x30, 0x18, 0x18, 0x18, 0x18, 0x18, 0xF0, 0xE0, 0x00, 0x00, 0x00, 0x00},
            {0x00, 0x00, 0x0F, 0x1F, 0x38, 0x30, 0x30, 0x30, 0x30, 0x38, 0x1F, 0x0F, 0x00, 0x00, 0x00, 0x00}
        },
        { // 7
            {0x00, 0x00, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0xFC, 0xFC, 0x00, 0x00, 0x00, 0x00},
            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xE0, 0xF8, 0x1F, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
            {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x3F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
        },
        { // 8
            {0x00, 0x00, 0xF0, 0xF8, 0x1C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1C, 0xF8, 0xF0, 0x00, 0x00, 0x00, 0x00},
            {0x00, 0x00, 0xE7, 0xFF, 0x3C, 0x18, 0x18, 0x18, 0x18, 0x3C, 0xFF, 0xE7, 0x00, 0x00, 0x00, 0x00},
            {0x00, 0x00, 0x0F, 0x1F, 0x38, 0x30, 0x30, 0x30, 0x30, 0x38, 0x1F, 0x0F, 0x00, 0x00, 0x00, 0x00}
        },
        { // 9
            {0x00, 0x00, 0xF0, 0xF8, 0x1C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1C, 0xF8, 0xF0, 0x00, 0x00, 0x00, 0x00},
            {0x00, 0x00, 0x07, 0x0F, 0x18, 0x18, 0x18, 0x18, 0x18, 0x0C, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00},
            {0x00, 0x00, 0x00, 0x30, 0x30, 0x30, 0x30, 0x30, 0x38, 0x1C, 0x0F, 0x07, 0x00, 0x00, 0x00, 0x00}
        }
    };  
    if (digit < 10) {
        for (int page = 0; page < 3; page++) {
            for (int col = 0; col < digit_width; col++) {
                data[1 + page * digit_width + col] = digit_patterns[digit][page][col];
            }
        }
    } else {
        memset(&data[1], 0, sizeof(data)-1);
    }
    i2c_write_blocking(I2C_PORT, OLED_ADDRESS, data, sizeof(data), false);
}

// Função para desenhar dois pontos no display OLED
void draw_colon(int x, int y) {
    uint8_t cmd[] = {0x00, 0x21, x, x + 7, 0x22, y / 8, y / 8 + 2};
    i2c_write_blocking(I2C_PORT, OLED_ADDRESS, cmd, sizeof(cmd), false);

    uint8_t data[1 + 8 * 3] = {0x40};
    uint8_t colon_pattern[3][8] = {
        {0x00, 0x00, 0x00, 0xC0, 0xC0, 0x00, 0x00, 0x00},
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0x00, 0x00, 0x00, 0x03, 0x03, 0x00, 0x00, 0x00}
    };

    for (int page = 0; page < 3; page++) {
        for (int col = 0; col < 8; col++) {
            data[1 + page * 8 + col] = colon_pattern[page][col];
        }
    }
    i2c_write_blocking(I2C_PORT, OLED_ADDRESS, data, sizeof(data), false);
}

// Exibe o tempo restante do Pomodoro no display OLED
void display_pomodoro_time() {
    static uint8_t last_minutes = 0xFF;
    static uint8_t last_seconds = 0xFF;
    static pomodoro_state_t last_state = POMODORO_STATE_IDLE;

    uint8_t minutes = pomodoro.remaining_time / 60;
    uint8_t seconds = pomodoro.remaining_time % 60;

    bool time_changed = (minutes != last_minutes) || (seconds != last_seconds);
    bool state_changed = (pomodoro.state != last_state);

    if (!time_changed && !state_changed)
        return;

    if (state_changed)
        display_bitmap();

    int base_x = 10;
    int base_y = 23;

    if ((minutes / 10) != (last_minutes / 10) || state_changed)
        draw_large_digit(minutes / 10, base_x, base_y);
    if ((minutes % 10) != (last_minutes % 10) || state_changed)
        draw_large_digit(minutes % 10, base_x + 18, base_y);
    if (last_minutes == 0xFF || state_changed)
        draw_colon(base_x + 36, base_y);
    if ((seconds / 10) != (last_seconds / 10) || state_changed)
        draw_large_digit(seconds / 10, base_x + 44, base_y);
    if ((seconds % 10) != (last_seconds % 10) || state_changed)
        draw_large_digit(seconds % 10, base_x + 62, base_y);

    last_minutes = minutes;
    last_seconds = seconds;
    last_state = pomodoro.state;
}

// Funções para desenhar texto no display
void draw_char(char c, int x, int y) {
    uint8_t ch_index;
    if (c == ' ') {
        ch_index = 0;
    } else if (c >= '0' && c <= '9') {
        ch_index = c - '0' + 1;  // dígitos começam no índice 1
    } else if (c >= 'A' && c <= 'Z') {
        ch_index = c - 'A' + 11; // letras maiúsculas começam no índice 11
    } else if (c >= 'a' && c <= 'z') {
        ch_index = c - 'a' + 11; 
    } else if(c == '>'){
        ch_index = 37;
    } else if(c == '<'){
        ch_index = 38;
    } else if(c == ':'){
        ch_index = 39;
    } else {
        ch_index = 0;
    }
    const uint8_t *bitmap = &font[ch_index * 8];

    uint8_t cmd[] = {0x00, 0x21, x, x + 7, 0x22, y / 8, y / 8};
    i2c_write_blocking(I2C_PORT, OLED_ADDRESS, cmd, sizeof(cmd), false);

    uint8_t data[9];
    data[0] = 0x40;
    for (int i = 0; i < 8; i++) {
        data[i+1] = bitmap[i];
    }
    i2c_write_blocking(I2C_PORT, OLED_ADDRESS, data, sizeof(data), false);
}

void draw_text(const char *text, int x, int y) {
    while (*text) {
        draw_char(*text, x, y);
        x += 9;  // 8 pixels para o caractere + 1 pixel de espaçamento
        text++;
    }
}

// Limpa o display OLED
void clear_display() {
    uint8_t cmd[] = {0x00, 0x21, 0, 127, 0x22, 0, 7};
    i2c_write_blocking(I2C_PORT, OLED_ADDRESS, cmd, sizeof(cmd), false);

    uint8_t clear_data[1025] = {0x40};
    i2c_write_blocking(I2C_PORT, OLED_ADDRESS, clear_data, 1025, false);
}

// Desenha o menu na tela
void draw_menu() {
    clear_display();

    switch (menu_state) {
        case MENU_STATE_MAIN:
            draw_text("POMODORO", 14, 0);
            draw_text(menu_selection == 0 ? "> INICIAR" : "  INICIAR", 0, 20);
            draw_text(menu_selection == 1 ? "> TEMPO TRAB" : "  TEMPO TRAB", 0, 30);
            draw_text(menu_selection == 2 ? "> PAUSA CURTA" : "  PAUSA CURTA", 0, 35);
            draw_text(menu_selection == 3 ? "> PAUSA LONGA" : "  PAUSA LONGA", 0, 45);
            draw_text("USE JOYSTICK", 16, 58);
            break;

        case MENU_STATE_WORK_TIME: {
            draw_text("TEMPO TRABALHO", 0, 0);
            char work_str[20];
            sprintf(work_str, "%d MINUTOS", user_work_time / 60);
            draw_text(work_str, 20, 20);
            draw_text("< MENOS MAIS >", 0, 40);
            draw_text("BTN: VOLTAR", 0, 56);
            break;
        }
        case MENU_STATE_SHORT_BREAK: {
            draw_text("PAUSA CURTA", 10, 0);
            char short_str[20];
            sprintf(short_str, "%d MINUTOS", user_short_break / 60);
            draw_text(short_str, 20, 20);
            draw_text("< MENOS MAIS >", 0, 40);
            draw_text("BTN: VOLTAR", 0, 56);
            break;
        }
        case MENU_STATE_LONG_BREAK: {
            draw_text("PAUSA LONGA", 10, 0);
            char long_str[20];
            sprintf(long_str, "%d MINUTOS", user_long_break / 60);
            draw_text(long_str, 20, 20);
            draw_text("< MENOS MAIS >", 0, 40);
            draw_text("BTN: VOLTAR", 0, 56);
            break;
        }
        case MENU_STATE_START:
            draw_text("INICIANDO...", 20, 20);
            draw_text("POMODORO", 30, 40);
            break;
    }
}

// Processa a entrada do joystick para navegar no menu
void process_joystick_menu() {
    static uint64_t last_action_time = 0;
    uint64_t current_time = time_us_64();

    if (current_time - last_action_time < 200000) return;  // debounce de 200 ms

    adc_select_input(0);
    uint16_t adc_x = adc_read();
    adc_select_input(1);
    uint16_t adc_y = adc_read();

    bool joystick_up = (adc_y < 1000);
    bool joystick_down = (adc_y > 3000);
    bool joystick_left = (adc_x < 1000);
    bool joystick_right = (adc_x > 3000);
    bool joystick_button = !gpio_get(JOYSTICK_BTN);

    bool action_taken = false;

    switch (menu_state) {
        case MENU_STATE_MAIN:
            if (joystick_right && menu_selection > 0) {
                menu_selection--;
                action_taken = true;
            } else if (joystick_left && menu_selection < 3) {
                menu_selection++;
                action_taken = true;
            } else if (joystick_button) {
                if (menu_selection == 0)
                    menu_state = MENU_STATE_START;
                else if (menu_selection == 1)
                    menu_state = MENU_STATE_WORK_TIME;
                else if (menu_selection == 2)
                    menu_state = MENU_STATE_SHORT_BREAK;
                else if (menu_selection == 3)
                    menu_state = MENU_STATE_LONG_BREAK;
                action_taken = true;
            }
            break;
        case MENU_STATE_WORK_TIME:
            if (joystick_left && user_work_time > 5 * 60) {
                user_work_time -= 5 * 60;
                action_taken = true;
            } else if (joystick_right && user_work_time < 60 * 60) {
                user_work_time += 5 * 60;
                action_taken = true;
            } else if (joystick_button) {
                menu_state = MENU_STATE_MAIN;
                action_taken = true;
            }
            break;
        case MENU_STATE_SHORT_BREAK:
            if (joystick_left && user_short_break > 60) {
                user_short_break -= 60;
                action_taken = true;
            } else if (joystick_right && user_short_break < 10 * 60) {
                user_short_break += 60;
                action_taken = true;
            } else if (joystick_button) {
                menu_state = MENU_STATE_MAIN;
                action_taken = true;
            }
            break;
        case MENU_STATE_LONG_BREAK:
            if (joystick_left && user_long_break > 5 * 60) {
                user_long_break -= 5 * 60;
                action_taken = true;
            } else if (joystick_right && user_long_break < 30 * 60) {
                user_long_break += 5 * 60;
                action_taken = true;
            } else if (joystick_button) {
                menu_state = MENU_STATE_MAIN;
                action_taken = true;
            }
            break;
        case MENU_STATE_START:
            menu_active = false;
            pomodoro_start(POMODORO_STATE_WORK);
            action_taken = true;
            break;
    }

    if (action_taken) {
        last_action_time = current_time;
        draw_menu();
    }
}



// Função de interrupção do botão para reiniciar o dispositivo
void gpio_irq_handler(uint gpio, uint32_t events) {
    reset_usb_boot(0, 0);
}

// Função de teste do buzzer passivo
void buzzer_test() {
    printf("\n*** TESTE DO BUZZER PASSIVO ***\n");
    
    for (int freq = 500; freq <= 2500; freq += 500) {
        printf("Testando frequência: %dHz\n", freq);
        buzzer_play(freq);
        sleep_ms(300);
        buzzer_play(0);
        sleep_ms(100);
    }
    
    // Padrão de alarme
    printf("Testando padrão de alarme\n");
    for (int i = 0; i < 5; i++) {
        buzzer_play(2000);
        sleep_ms(100);
        buzzer_play(1000);
        sleep_ms(100);
    }
    buzzer_play(0);
    
    printf("Teste do buzzer concluído\n");
}

// Função principal
int main() {
    stdio_init_all();
    set_sys_clock_khz(128000, true);

    // Aguardar inicialização completa
    sleep_ms(1000);
    printf("\n\n*** INICIANDO PROGRAMA POMODORO ***\n");
    
    // Inicializa o buzzer primeiro
    buzzer_init();
    
    // Teste imediatamente após inicialização
    buzzer_test();

    oled_init();
    
    
    adc_init();
    adc_gpio_init(JOYSTICK_X_PIN);
    adc_gpio_init(JOYSTICK_Y_PIN);

    gpio_init(JOYSTICK_BTN);
    gpio_set_dir(JOYSTICK_BTN, GPIO_IN);
    gpio_pull_up(JOYSTICK_BTN);

    PIO pio = pio0;
    uint offset = pio_add_program(pio, &pio_matrix_program);
    uint sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, OUT_PIN);

    gpio_init(button_0);
    gpio_set_dir(button_0, GPIO_IN);
    gpio_pull_up(button_0);
    gpio_set_irq_enabled_with_callback(button_0, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    printf("Programa iniciado.\n");

    uint32_t buffer_leds[NUM_PIXELS];

    draw_menu();

    while (true) {
        buzzer_update(); // Atualiza o buzzer em cada ciclo

        if (menu_active) {
            process_joystick_menu();

            // Atualiza a matriz LED para indicar modo de menu
            for (int i = 0; i < NUM_PIXELS; i++) {
                buffer_leds[i] = matrix_rgb(0.0, 0.0, 0.1);
            }
            for (int i = 0; i < NUM_PIXELS; i++) {
                pio_sm_put_blocking(pio, sm, buffer_leds[mapa_leds[i]]);
            }
        } else {
            pomodoro_update();

            display_pomodoro_time();

            for (int i = 0; i < NUM_PIXELS; i++) {
                buffer_leds[i] = get_color(i);
            }
            for (int i = 0; i < NUM_PIXELS; i++) {
                pio_sm_put_blocking(pio, sm, buffer_leds[mapa_leds[i]]);
            }

            static uint32_t button_hold_time = 0;
            if (!gpio_get(JOYSTICK_BTN)) {
                button_hold_time++;
                if (button_hold_time > 20) {
                    menu_active = true;
                    menu_state = MENU_STATE_MAIN;
                    button_hold_time = 0;
                    draw_menu();
                }
            } else {
                button_hold_time = 0;
            }
        }
        sleep_ms(100);
    }

    return 0;
}
