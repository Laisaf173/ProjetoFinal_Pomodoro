# Pomodoro Digital - Projeto com Raspberry Pi Pico
Este é um dispositivo Pomodoro digital implementado usando Raspberry Pi Pico, que combina display OLED, matriz LED RGB e feedback sonoro para auxiliar na gestão de tempo usando a técnica Pomodoro.

### Características Principais
- Display OLED para visualização do tempo
- Matriz LED RGB 5x5 em formato de tomate
- Buzzer para alertas sonoros
- Interface de controle via joystick
- Menu interativo para configuração

--> Temporizações configuráveis:
- Tempo de trabalho (padrão: 25 min)
- Pausa curta (padrão: 5 min)
- Pausa longa (padrão: 15 min)

### Funcionalidades
--> Menu Principal
- Iniciar Pomodoro
- Configurar tempo de trabalho
- Configurar pausa curta
- Configurar pausa longa

--> Durante o Pomodoro
- Display mostra contagem regressiva
- LEDs indicam progresso visualmente
- Alarme sonoro nas transições
- Pausa longa após 4 ciclos
- Retorno ao menu via botão reset 
- Feedback Visual

--> Matriz LED em forma de tomate
- LEDs se apagam gradualmente conforme o tempo passa
- Cores diferentes para trabalho/pausa

### Como Usar
- Conecte o dispositivo à energia
- Use o joystick para navegar no menu
- Configure os tempos desejados
- Selecione "INICIAR" para começar
- O dispositivo alternará automaticamente entre períodos de trabalho e pausa
- Use o botão de reset para reiniciar o sistema
