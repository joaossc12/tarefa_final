#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "./includes/ssd1306.h"
#include "./includes/font.h"
#include "hardware/pio.h"
#include "tarefa_final.pio.h"

//---------------------------------------------------
// DEFINES
//----------------------------------------------------

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define LED_RED 13
#define LED_BLUE 12
#define LED_GREEN 11
#define BUTTON_A 5
#define BUTTON_B 6     
#define JOYSTICK_X 27  
#define JOYSTICK_Y 26  
#define JOYSTICK_BT 22 

//---------------------------------------------------
// VARIÁVEIS GLOBAIS
//----------------------------------------------------

uint16_t adc_value_x;
uint16_t adc_value_y;  
uint slice_led; //GPIO 13 e 12 controladas pelo mesmo slice
uint16_t wrap = 2048; //Escolhido para facilitar as contas
float div_clk = 30; //Para aproximadamente 2khz
uint16_t center_value = 2000;//Medido Experimentalmente
ssd1306_t ssd; //Display de 128 x 64


//---------------------------------------------------
// PROTOTIPAÇÃO
//----------------------------------------------------

static void callback_button(uint gpio, uint32_t events);

int main()
{
    stdio_init_all();

    while (true) {
        printf("Hello, world!\n");
        sleep_ms(1000);
    }
}
static void callback_button(uint gpio, uint32_t events) {
    static absolute_time_t last_time_A = 0; // Tempo do último evento do botão A
    static absolute_time_t last_time_B = 0; // Tempo do último evento do botão B
    static absolute_time_t last_time_J = 0; // Tempo do último evento do botão Joystick

    absolute_time_t now = get_absolute_time();

    if (gpio == BUTTON_A) { // Interrupção do botão A
        if (absolute_time_diff_us(last_time_A, now) > 200000) { // Debounce de 200ms

            last_time_A = now; // Atualiza o tempo do último evento do botão A
        }
    } else if (gpio == BUTTON_A) { // Interrupção do botão B
        if (absolute_time_diff_us(last_time_B, now) > 200000) { // Debounce de 200ms

            last_time_B = now; // Atualiza o tempo do último evento do botão B
        }
    } else if (gpio == JOYSTICK_BT){ //Interrupção botão joystick
                if (absolute_time_diff_us(last_time_J, now) > 200000) { // Debounce de 200ms

            last_time_J = now; // Atualiza o tempo do último evento do botão B
        }

    }
}