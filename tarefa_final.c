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
#include "hardware/timer.h"
#include "hardware/clocks.h"

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
#define BUZZER 21
#define LED_MATRIX 7
//---------------------------------------------------
// VARIÁVEIS GLOBAIS
//----------------------------------------------------

//ADC
uint16_t adc_value_x;
uint16_t adc_value_y;  
uint16_t center_value = 2000;//Medido Experimentalmente
//PWM
uint slice; //GPIO 13 e 12 controladas pelo mesmo slice
uint16_t wrap = 2048; //Escolhido para facilitar as contas
float div_clk = 30; //Para aproximadamente 2khz
//SSD
ssd1306_t ssd; //Display de 128 x 64

//Tanque
double volume;
double volume_max = 1000; //Volume máximo do tanque em L
double vel_input = 100; //Vazão de entrada 
double vel_output = 50; //Vazão de saída
double vel_max = 10; //Vazão maxima de entrada ou saída
bool flag_input = 0; //Estado da valvula de entrada
bool flag_output = 0; //Estado da valvula de saída
bool flag_max = 0;
uint16_t sample_time = 500; //Periodo de amostragem em ms




//---------------------------------------------------
// PROTOTIPAÇÃO
//----------------------------------------------------

static void callback_button(uint gpio, uint32_t events);
bool atualiza_dados(struct repeating_timer *t);
void init_pinos();

int main()
{
    stdio_init_all();
    init_pinos();
    //Timer
    struct repeating_timer timer;
    add_repeating_timer_ms(sample_time, atualiza_dados, NULL, &timer);
    //ISR
    gpio_set_irq_enabled_with_callback(JOYSTICK_BT, GPIO_IRQ_EDGE_FALL, true, callback_button);
    gpio_set_irq_enabled(BUTTON_A, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BUTTON_B, GPIO_IRQ_EDGE_FALL, true);
    //ADC
    adc_init();
    adc_gpio_init(JOYSTICK_X);
    adc_gpio_init(JOYSTICK_Y); 
    //PWM
    pwm_set_clkdiv(slice, div_clk); //define o divisor de clock do PWM
    pwm_set_wrap(slice, wrap); //definir o valor de wrap – valor máximo do contador PWM
    pwm_set_enabled(slice, true); //habilitar o pwm no slice correspondent
    //Display
    i2c_init(I2C_PORT, 1000 * 1000); //Frquencia de 400Khz
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
    ssd1306_config(&ssd); // Configura o display
    ssd1306_send_data(&ssd); // Envia os dados para o display
    ssd1306_fill(&ssd, false); //Apaga todos pixels
    ssd1306_send_data(&ssd); //Envia o comando
    //PIO
    PIO pio = pio0;
    bool clk = set_sys_clock_khz(128000, false);
    uint offset = pio_add_program(pio, &tafera_final_program);
    uint sm = pio_claim_unused_sm(pio, true);
    tafera_final_program_init(pio, sm, offset, LED_MATRIX);

    while (true) {
        tight_loop_contents();
    }
}
static void callback_button(uint gpio, uint32_t events) {
    static absolute_time_t last_time_A = 0; // Tempo do último evento do botão A
    static absolute_time_t last_time_B = 0; // Tempo do último evento do botão B
    static absolute_time_t last_time_J = 0; // Tempo do último evento do botão Joystick

    absolute_time_t now = get_absolute_time();

    if (gpio == BUTTON_A) { // Interrupção do botão A
        if (absolute_time_diff_us(last_time_A, now) > 200000) { // Debounce de 200ms
            flag_input = !flag_input;
            printf("Estado da entrada %b \n", flag_input);
            last_time_A = now; // Atualiza o tempo do último evento do botão A
        }
    } else if (gpio == BUTTON_B) { // Interrupção do botão B
        if (absolute_time_diff_us(last_time_B, now) > 200000) { // Debounce de 200ms
            flag_output = !flag_output;
            printf("Estado da saída %b \n", flag_output);
            last_time_B = now; // Atualiza o tempo do último evento do botão B
        }
    } else if (gpio == JOYSTICK_BT){ //Interrupção botão joystick
                if (absolute_time_diff_us(last_time_J, now) > 200000) { // Debounce de 200ms

            last_time_J = now; // Atualiza o tempo do último evento do botão B
        }

    }
}

bool atualiza_dados(struct repeating_timer *t) {

    if (volume >= volume_max){
        gpio_set_irq_enabled(BUTTON_A, GPIO_IRQ_EDGE_FALL, false);
        flag_max = 1;
        flag_input = 0;
    }else{
        gpio_set_irq_enabled(BUTTON_A, GPIO_IRQ_EDGE_FALL, true);
    }
    volume = volume + (flag_input * vel_input) - (flag_output * vel_output);
    printf("Volume = %f \n", volume);

}
void init_pinos(){
  gpio_set_function(BUZZER, GPIO_FUNC_PWM); //habilitar o pino GPIO como PWM
  slice = pwm_gpio_to_slice_num(BUZZER); //obter o canal (slice) PWM da GPIO

  gpio_init(LED_GREEN);
  gpio_set_dir(LED_GREEN, GPIO_OUT);
  gpio_init(LED_RED);
  gpio_set_dir(LED_RED, GPIO_OUT);
  gpio_init(LED_BLUE);
  gpio_set_dir(LED_BLUE, GPIO_OUT);


  gpio_init(BUTTON_A);//inicializa o pino 5 do microcontrolador
  gpio_set_dir(BUTTON_A, GPIO_IN);//configura o pino como entrada
  gpio_pull_up(BUTTON_A); //Pull up pino 5

  gpio_init(BUTTON_B);//inicializa o pino 6 do microcontrolador
  gpio_set_dir(BUTTON_B, GPIO_IN);//configura o pino como entrada
  gpio_pull_up(BUTTON_B); //Pull up pino 6

  gpio_init(JOYSTICK_BT);//inicializa o pino 22 do microcontrolador
  gpio_set_dir(JOYSTICK_BT, GPIO_IN);//configura o pino como entrada
  gpio_pull_up(JOYSTICK_BT); //Pull up pino 22

gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
gpio_pull_up(I2C_SDA); // Pull up the data line
gpio_pull_up(I2C_SCL); // Pull up the clock line

}
