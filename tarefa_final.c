#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
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
//---------------------------------------------------
#define I2C_PORT        i2c1
#define I2C_SDA         14
#define I2C_SCL         15
#define ENDERECO        0x3C
#define LED_RED         13
#define LED_BLUE        12
#define BUTTON_A        5
#define BUTTON_B        6     
#define JOYSTICK_X      27  
#define JOYSTICK_Y      26  
#define JOYSTICK_BT     22 
#define BUZZER          21
#define LED_MATRIX      7
#define TOTAL_LEDS      25
#define MAX_LED_VALUE   60

#define MAX_FUNC(a, b)  (((a) >= (b)) ? (a) : (b))
#define MIN_FUNC(a, b)  (((a) <= (b)) ? (a) : (b))

//---------------------------------------------------
// VARIÁVEIS GLOBAIS
//---------------------------------------------------
// ADC
uint16_t adc_value_x;
uint16_t adc_value_y;  
uint16_t center_value = 2000;  // Valor medido experimentalmente

// PWM BUZZER
uint slice; 
const uint16_t wrap = 1000;   // Valor de wrap do PWM
// PWM LED - frequência de 10khz
uint slice_led;
const uint16_t wrap_led = 255;
const float div_clock_led = 50.0;

// Display SSD1306
ssd1306_t ssd; // Display de 128 x 64

// Controle do tanque

volatile double volume = 480;          // Volume inicial
const double volume_max = 1200;        // Volume máximo (mL)
volatile double v_input = 30;         // Vazão de entrada
volatile double v_output = 30;         // Vazão de saída
const double vel_max = 60;               // Vazão máxima mL/s
volatile bool flag_input = false;          // Estado da bomba de entrada
volatile bool flag_output = false;         // Estado da bomba de saída
volatile bool flag_max = false;
volatile bool flag_min = false;
volatile bool flag_perigo = false;         // Indica situação crítica (volume no limite)
volatile bool flag_adc = true;             //Indica o estado da leitura do ADC
const uint16_t sample_time = 100;         // Período de amostragem (ms)

//---------------------------------------------------
// PROTOTIPAÇÃO
//---------------------------------------------------
static void callback_button(uint gpio, uint32_t events);
bool atualiza_dados(struct repeating_timer *t);
void init_pinos(void);
void controle_vazao(int adc_x, int adc_y);
void controle_matrix(PIO pio, uint sm, double volume, bool flag_perigo);
void controle_display(void);
void controle_buzzer(uint slice);
void controle_atuador(double vel_input, double vel_output);

//---------------------------------------------------
// FUNÇÃO MAIN
//---------------------------------------------------
int main() {
    stdio_init_all();
    init_pinos();

    // Configura o timer para atualização periódica dos dados
    struct repeating_timer timer;
    add_repeating_timer_ms(sample_time, atualiza_dados, NULL, &timer);

    // Configura ISR para os botões e o joystick
    gpio_set_irq_enabled_with_callback(JOYSTICK_BT, GPIO_IRQ_EDGE_FALL, true, callback_button);
    gpio_set_irq_enabled(BUTTON_A, GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(BUTTON_B, GPIO_IRQ_EDGE_FALL, true);

    // Inicializa o ADC e configura os pinos correspondentes
    adc_init();
    adc_gpio_init(JOYSTICK_X);
    adc_gpio_init(JOYSTICK_Y); 

    // Configura o PWM para o BUZZER e ATUADOR
    pwm_set_enabled(slice, true);
    pwm_set_wrap(slice, wrap);
    
    // Configura o PWM para o ATUADOR/LED
    pwm_set_enabled(slice_led, true);
    pwm_set_wrap(slice_led, wrap_led);
    pwm_set_clkdiv(slice_led, div_clock_led);
    

    // Inicializa o display via I2C
    i2c_init(I2C_PORT, 400000); // 0.4 MHz
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORT);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    // Configura o PIO para controlar a matriz de LEDs
    PIO pio = pio0;
    bool clk = set_sys_clock_khz(128000, false);
    uint offset = pio_add_program(pio, &tafera_final_program);
    uint sm_pio = pio_claim_unused_sm(pio, true);
    tafera_final_program_init(pio, sm_pio, offset, LED_MATRIX);

    // Laço principal
    while (true) {
        // Leitura dos ADCs
        adc_select_input(1); 
        adc_value_x = adc_read();
        adc_select_input(0);
        adc_value_y = adc_read();  
        
        // Atualiza a vazão com base nos ADCs
        controle_vazao(adc_value_x, adc_value_y);
        //Atualiza o PWM do atuador conforme niveis estabelecidos
        controle_atuador(v_input,v_output);
        // Atualiza o display com os dados atuais
        controle_display();
        // Atualiza a matriz de LEDs com o volume atual e situação de perigo
        controle_matrix(pio, sm_pio, volume, flag_perigo);
        // Atualiza o buzzer conforme as condições de segurança
        controle_buzzer(slice);
    }

    return 0;
}

//---------------------------------------------------
// FUNÇÃO DE CALLBACK DOS BOTÕES
//---------------------------------------------------
static void callback_button(uint gpio, uint32_t events) {
    static absolute_time_t last_time_A = 0;
    static absolute_time_t last_time_B = 0;
    static absolute_time_t last_time_J = 0;
    absolute_time_t now = get_absolute_time();

    if (gpio == BUTTON_A) {
        if (absolute_time_diff_us(last_time_A, now) > 200000) {
            flag_input = !flag_input;
            printf("Estado da entrada: %d\n", flag_input);
            last_time_A = now;
        }
    } else if (gpio == BUTTON_B) {
        if (absolute_time_diff_us(last_time_B, now) > 200000) {
            flag_output = !flag_output;
            printf("Estado da saída: %d\n", flag_output);
            last_time_B = now;
        }
    } else if (gpio == JOYSTICK_BT) {
        if (absolute_time_diff_us(last_time_J, now) > 200000) {

        
            last_time_J = now;
        }
    }
}

//---------------------------------------------------
// FUNÇÃO DE ATUALIZAÇÃO DOS DADOS (Timer)
//---------------------------------------------------
bool atualiza_dados(struct repeating_timer *t) {
    // Controle de segurança: se volume atinge os limites, desabilita a válvula correspondente
    if (volume >= volume_max) {
        gpio_set_irq_enabled(BUTTON_A, GPIO_IRQ_EDGE_FALL, false);
        flag_max = true;
        flag_input = false;
        printf("Entrada desligada por segurança!\n");
    } else if (volume <= 0) {
        gpio_set_irq_enabled(BUTTON_B, GPIO_IRQ_EDGE_FALL, false);
        flag_output = false;
        flag_min = true;
        printf("Saída desligada por segurança!\n");
    } else {
        flag_min = false;
        flag_max = false;
        gpio_set_irq_enabled(BUTTON_A, GPIO_IRQ_EDGE_FALL, true);
        gpio_set_irq_enabled(BUTTON_B, GPIO_IRQ_EDGE_FALL, true);
    }
    flag_perigo = flag_max || flag_min;
    
    // Calcula a variação de volume (delta) considerando o tempo de amostragem (em segundos)
    double delta_time = (double)sample_time / 1000.0;
    double delta_volume = ((flag_input ? v_input : 0) - (flag_output ? v_output : 0)) * delta_time;
    volume += delta_volume;
    volume = MAX_FUNC(MIN_FUNC(volume, volume_max), 0);

    printf("Volume = %f | Entrada: %d | Saída: %d\n", volume, flag_input, flag_output);
    return true;
}

//---------------------------------------------------
// FUNÇÃO DE INICIALIZAÇÃO DOS PINOS
//---------------------------------------------------
void init_pinos(void) {
    // Configura o BUZZER para PWM
    gpio_set_function(BUZZER, GPIO_FUNC_PWM);
    slice = pwm_gpio_to_slice_num(BUZZER);

    // Inicializa LEDs 
    gpio_set_function(LED_RED, GPIO_FUNC_PWM);
    gpio_set_function(LED_BLUE, GPIO_FUNC_PWM);
    slice_led = pwm_gpio_to_slice_num(LED_RED);


    // Inicializa os botões com pull-up
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);

    gpio_init(JOYSTICK_BT);
    gpio_set_dir(JOYSTICK_BT, GPIO_IN);
    gpio_pull_up(JOYSTICK_BT);

    // Configura I2C para o display
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
}

//---------------------------------------------------
// FUNÇÃO DE CONTROLE DA VAZÃO (ADC)
//---------------------------------------------------
void controle_vazao(int adc_x, int adc_y) {
    // Elimina ruídos: se a diferença for menor que a tolerância, usa o valor central
    uint16_t tolerancia = 100;
    uint16_t leitura_x = (abs(adc_x - center_value) > tolerancia) ? adc_x : center_value;
    uint16_t leitura_y = (abs(adc_y - center_value) > tolerancia) ? adc_y : center_value;
    
    // Converte a variação do ADC em incremento para a vazão (escala aproximada de [-10,10])
    flag_adc = !flag_input & !flag_output;
    if (!flag_input){
        double conv_input = (double)(leitura_x - center_value) * 0.00048852*2;
        v_input += conv_input;
        v_input = MAX_FUNC(MIN_FUNC(v_input, vel_max), 0);
        printf("Velocidade de entrada: %f\n", v_input);
    }
    if (!flag_output){
        double conv_output = (double)(leitura_y - center_value) * 0.00048852*2;
        v_output += conv_output;
        v_output = MAX_FUNC(MIN_FUNC(v_output, vel_max), 0);
        printf("Velocidade de saída: %f\n", v_output);
    }
}

//---------------------------------------------------
// FUNÇÃO DE CONTROLE DA MATRIZ DE LEDS
//---------------------------------------------------
void controle_matrix(PIO pio, uint sm, double volume, bool flag_perigo) {
    int8_t cor_steps = (flag_perigo) ? 16 : 8;
    // Mapeia volume [0, volume_max] para [0, TOTAL_LEDS] (valor fracionário)
    double leds_ativos = (TOTAL_LEDS * volume) / volume_max;
    int leds_full = (int)leds_ativos;
    double led_parcial = fmod(leds_ativos, 1.0);
    int brilho_parcial = (int)(led_parcial * MAX_LED_VALUE);
    
    uint32_t valor_full = ((uint32_t)MAX_LED_VALUE) << cor_steps;
    uint32_t valor_parcial = ((uint32_t)brilho_parcial) << cor_steps;
    
    uint32_t valor[TOTAL_LEDS];
    for (int i = 0; i < TOTAL_LEDS; i++) {
        if (i < leds_full) {
            valor[i] = valor_full;
        } else if (i == leds_full && (led_parcial > 0.0)) {
            valor[i] = valor_parcial;
        } else {
            valor[i] = 0;
        }
    }
    for (int i = 0; i < TOTAL_LEDS; i++) {
        pio_sm_put_blocking(pio, sm, valor[i]);
    }
}

//---------------------------------------------------
// FUNÇÃO DE CONTROLE DO DISPLAY
//---------------------------------------------------
void controle_display(void) {
    uint16_t volume_p100 = (uint16_t)(100 * (volume / volume_max));
    uint16_t vel_input_p100 = (uint16_t)(100 * (v_input / vel_max));
    uint16_t vel_output_p100 = (uint16_t)(100 * (v_output / vel_max));

    char exibir_volume[4];
    char exibir_input[4];
    char exibir_output[4];
    sprintf(exibir_volume, "%d", volume_p100);
    sprintf(exibir_input, "%d", vel_input_p100);
    sprintf(exibir_output, "%d", vel_output_p100);

    ssd1306_fill(&ssd, false);
    ssd1306_rect(&ssd, 0, 0, 126, 62, 1, 0);
    ssd1306_draw_string(&ssd, "IHM CONTROLE", 10, 2);
    ssd1306_draw_string(&ssd, "VOLUME=", 4, 14);
    ssd1306_draw_string(&ssd, exibir_volume, 88, 14);
    ssd1306_draw_char(&ssd, '%', 112, 14);
    ssd1306_draw_string(&ssd, "V.ENTRADA=", 4, 26);
    ssd1306_draw_string(&ssd, exibir_input, 88, 26);
    ssd1306_draw_char(&ssd, '%', 112, 26);
    ssd1306_draw_string(&ssd, "V.SAIDA=", 4, 38);
    ssd1306_draw_string(&ssd, exibir_output, 88, 38);
    ssd1306_draw_char(&ssd, '%', 112, 38);

    char msg[20];
    if (flag_max) {
        strcpy(msg, "TANQUE CHEIO");
    } else if (flag_min) {
        strcpy(msg, "TANQUE VAZIO");
    } else {
        sprintf(msg, "IN = %d OUT = %d", flag_input ? 1 : 0, flag_output ? 1 : 0);
    }
    // Centraliza a mensagem com base no tamanho (assumindo que cada caractere tem 6px de largura)
    int pos_x = 2;
    ssd1306_draw_string(&ssd, msg, pos_x, 50);
    ssd1306_send_data(&ssd);

    printf("PORCENTAGEM DE VOLUME: %d\n", volume_p100);
}

//---------------------------------------------------
// FUNÇÃO DE CONTROLE DO BUZZER (PWM)
//---------------------------------------------------
void controle_buzzer(uint slice) {
    // Se o volume estiver nos limites de segurança, emite alarme:
    if (flag_max) {
        // Alarme de emergência: tom mais agudo
        uint16_t level = 500;   // Duty cycle 50% (em wrap 1000)
        float div = 100.0f;     // Divisor para frequência mais alta
        pwm_set_gpio_level(BUZZER, level);
        pwm_set_clkdiv(slice, div);
    } else if (flag_min) {
        // Alarme de baixa emergência: tom mais grave
        uint16_t level = 500;
        float div = 250.0f;     // Divisor para frequência mais baixa
        pwm_set_gpio_level(BUZZER, level);
        pwm_set_clkdiv(slice, div);
    } else {
        // Sem alarme: buzzer desligado
        pwm_set_gpio_level(BUZZER, 0);
    }
}
//---------------------------------------------------
// FUNÇÃO PARA ATUALIZAR A POTÊNCIA DO ATUADOR
//---------------------------------------------------
void controle_atuador(double vel_input, double vel_output){
    uint16_t level_input = (uint16_t)255 * (vel_input/ vel_max);
    uint16_t level_output = (uint16_t)255 * (vel_output/ vel_max);

    pwm_set_gpio_level(LED_RED, level_input);
    pwm_set_gpio_level(LED_BLUE, level_output);

}