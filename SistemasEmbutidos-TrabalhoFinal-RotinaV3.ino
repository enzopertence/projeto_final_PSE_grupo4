#include <Arduino.h>
#include <math.h>
#include <LiquidCrystal.h>

#define BETA 4150
#define R0 10000

const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
double temp = 0;
int measurementsCount = 0;

void setup() {
  lcd.begin(16, 2); // Inicializa o display
  lcd.print("Pressione para");
  lcd.setCursor(0, 1);
  lcd.print("medir");
  

  cli();  // Limpa os registradores de interrupções

  DDRD = DDRD & ~(1 << DDD6);   // Coloca o pino 6 da porta D como entrada
  
  TCCR1A = 0; // Coloca timer em operação normal
  TCCR1B = 0; // Reseta registradores de modo e prescaler
  OCR1A = 0x1388; // Coloca valor de contagem para 25 ms = 40 Hz, logo OCR1A deve ser: (16*10^6) / (40*8) = 5000 = 0x1388
  TCNT1  = 0; // Zera contador do timer

  sei() ; // Habilita interrupções
}

void loop() {
  waitForButtonPulse(64, 64); // Espera pelo pulso de entrada no botão; 16 é o primeiro estado a checar porque se está lendo do pido D4 (2^4 = 16) e porque a tensão lida é nula com o botão liberado
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temperatura");
  lcd.setCursor(0, 1);  // Coloca cursor do display na segunda linha
  setOrResetTimer(true);  // Inicial a temporização e a obtenção das 10 amostras
}

void waitForButtonPulse(int firstStateToCheck, int HIGHstateValue) {
  bool pulseDetected = false;
  int buttonState, stateToCheck = firstStateToCheck;

  while (!pulseDetected) {

    buttonState = PIND & (1 << PD6);  // Leitura do registrador de entrada

    if (buttonState == stateToCheck) {  // Verificação de borda (de subida ou descida, dependendo do valor de "stateToCheck")

      stateToCheck = !stateToCheck ? HIGHstateValue : 0;

      if (stateToCheck == firstStateToCheck) {  // Se o próximo estado a verificar voltou a ser o inicial, então significa que um pulso completo ocorreu na entrada
        pulseDetected = 1;
      }
    }
  }
}

void setOrResetTimer(bool set) {
  if (set) {
    TCNT1 = 0;
    TCCR1B |= (1 << WGM12) | (1 << CS11); // Coloca timer em modo CTC (1 << WGM12), com prescaler de 8 (CS11 = 1)
    TIMSK1 |= (1 << OCIE1A);  // Habilita interrupção do timer por igualdade de comparação
  }
  else {
    TIMSK1 &= ~(1 << OCIE1A);
    TCCR1B = 0;
  }
}

double readNTC() {
  int input;
  double voltage, resistance, tempInK;

  ADMUX |= B01000000;   // Canal A0 e a define tensao de ref de VCC
  ADCSRA |= B11000000;  // Ativa o adc e comeca a conversao; temos que ativar o ADSC sempre pois o arduino desativa ele depois de cada conversão

  while (bit_is_set(ADCSRA, ADSC)) {
    input = ADCL | (ADCH << 8);
  }
  voltage = 0.004883 * input;   // (5V/1024)
  resistance = (R0 * voltage) / (5 - voltage); // Fórmula do divisor de tensão
  tempInK = 1 / (0.0033557 + (log(resistance / R0)) / BETA); // Fórmula do NTC, 0.0033557 eh 1/T0, T0 = 298K

  return tempInK - 273;    // temperatura em celsius
}

ISR(TIMER1_COMPA_vect) {
  if (measurementsCount < 10) { // Enquanto não tiverem sido medidas as 10 amostras, continuar medindo e calculando a média
    temp += readNTC() * 0.1; // média de 10 amostras da temperatura
    measurementsCount ++;
  }
  else {
    measurementsCount = 0;  // Zera o contador de amostras
    setOrResetTimer(false);
    lcd.print(temp);
    lcd.print("\337C");
    temp = 0;
  }
}
