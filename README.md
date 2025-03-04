﻿# Controle e Monitoramento de Nível em Tanques

## Descrição
Este projeto implementa um sistema de controle e monitoramento de nível em tanques utilizando a placa de desenvolvimento BitDogLab (baseada no microcontrolador RP2040) e o Pico SDK em linguagem C. O firmware integra diversas funcionalidades críticas, incluindo:
- **Leitura de Sensores:** Os valores analógicos dos eixos X e Y de um joystick são captados através dos conversores ADC para ajustar as vazões.
- **Controle de Atuadores:** As vazões de entrada e saída são controladas, e os atuadores (simulados por LEDs e um buzzer) são modulados via PWM para representar bombas ou válvulas em uma aplicação real.
- **Gerenciamento do Volume:** O volume do tanque é calculado dinamicamente com base na diferença entre as vazões de entrada e saída, utilizando mecanismos de saturação para manter os níveis dentro de limites pré-definidos.
- **Interface Homem-Máquina:** Informações do sistema são exibidas em um display OLED (SSD1306) e em uma matriz de LEDs, que proporcionam uma representação gráfica do nível do tanque. Adicionalmente, alarmes sonoros são emitidos pelo buzzer em situações críticas (nível máximo ou mínimo).

## Objetivos
- **Demonstrar a integração de sensores e atuadores** na Raspberry Pi Pico para o controle de um sistema de monitoramento de tanques.
- **Exibir informações visuais e feedback sonoro** através de um display OLED, matriz de LEDs e buzzer.
- **Explorar o uso de ADC, PWM, I2C, interrupções, temporizadores e PIO** para controle de hardware.
- **Implementar mecanismos de segurança** que travem ou liberem as válvulas (simuladas) conforme os limites de volume do tanque.

## Tecnologias Utilizadas
- **Linguagem de Programação:** C
- **Plataforma:** Raspberry Pi Pico (RP2040) – BitDogLab
- **Hardware Utilizado:**
  - **Sensores:** ADC para leitura do joystick (eixos X e Y)
  - **Atuadores:** Buzzer (PWM) e atuadores simulados (LEDs para representar bombas/ válvulas)
  - **Módulos de Interface Visual:** Display OLED (SSD1306) e matriz de LEDs
  - **Comunicação:** I2C para o display; PWM para controle de buzzer e atuadores; interrupções para botões e joystick
- **Bibliotecas:** Pico SDK, hardware/adc.h, hardware/i2c.h, hardware/pwm.h, hardware/pio.h, hardware/timer.h, hardware/clocks.h math.h sdtlib.h

## Vídeo de Demonstração
[Youtube - Demonstração do Projeto](https://youtu.be/OZ_uwak-14Y)  


## Como Executar o Projeto

### Pré-Requisitos

#### Hardware
- **Placa de Desenvolvimento:** Raspberry Pi Pico (ou BitDogLab)
- **Display OLED (SSD1306 – 128x64):** Conectado via I2C  
  - SDA: GPIO 14  
  - SCL: GPIO 15  
  - Endereço I2C: 0x3C
- **Joystick com dois eixos e botão integrado:**  
  - Eixo X: GPIO 27 (entrada ADC)  
  - Eixo Y: GPIO 26 (entrada ADC)  
  - Botão do Joystick: GPIO 22 (entrada digital com pull-up)
- **Atuadores e Indicadores:**  
  - LED Vermelho: GPIO 13 (configurado para PWM)  
  - LED Azul: GPIO 12 (configurado para PWM)  
  - Buzzer: GPIO 21 (PWM)  
  - Matriz de LEDs: Conectada conforme o design do projeto (GPIO 7)
- **Botões:**  
  - Botão A: GPIO 5 (entrada digital com pull-up)  
  - Botão B: GPIO 6 (entrada digital com pull-up)

#### Software
- Ambiente de desenvolvimento configurado com o SDK da Raspberry Pi Pico (C)
- Ferramentas de compilação: CMake e Make (ou similar)

### Passos para Execução

1. **Clone o Repositório:**  
   Faça o download ou clone o repositório que contém o código-fonte.

2. **Configure o Ambiente de Desenvolvimento:**  
   Certifique-se de que o SDK da Raspberry Pi Pico está instalado e configurado corretamente.

3. **Compilação:**  
   Utilize as ferramentas do SDK (CMake, Make) para compilar o código.

4. **Conexões de Hardware:**  
   Conecte os componentes conforme as definições abaixo:
   - **Display OLED (I2C):**  
     SDA → GPIO 14  
     SCL → GPIO 15  
     Endereço I2C: 0x3C
   - **Joystick:**  
     Eixo X → GPIO 27 (ADC)  
     Eixo Y → GPIO 26 (ADC)  
     Botão do Joystick → GPIO 22
   - **LEDs (Simulação de Bombas/Válvulas):**  
     LED Vermelho → GPIO 13 (PWM)  
     LED Azul → GPIO 12 (PWM)
   - **Buzzer (Alarme Sonoro):**  
     → GPIO 21 (PWM)
   - **Botões:**  
     Botão A → GPIO 5  
     Botão B → GPIO 6

5. **Carregue o Firmware:**  
   Transfira o binário compilado para a placa Raspberry Pi Pico utilizando o método adequado (por exemplo, arrastando o arquivo UF2 para a unidade montada).

6. **Execução:**  
   Após a transferência, o sistema inicia a leitura contínua dos sensores (ADC), atualiza as vazões e o volume do tanque, e exibe as informações na interface visual (display OLED e matriz de LEDs). Além disso, o buzzer emite alarmes sonoros conforme as condições de segurança (tanque cheio ou vazio).

