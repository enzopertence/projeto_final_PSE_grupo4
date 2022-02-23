#define BETA 4150
#define R0 10000
#include <Arduino.h>
#include <math.h>

double ler_NTC() {
  ADMUX |= B01000000;               // canal A0 e a define tensao de ref de VCC
  
  ADCSRA |= B11000000;              // ativa o adc e comeca a conversao
                                    // temos que ativar o ADSC sempre pois o arduino desativa ele 
                                    // depois de cada conversao
  int leitura;                      

  while (bit_is_set(ADCSRA, ADSC)) {
    leitura = ADCL | (ADCH << 8);
  }
  
  double tensao = 0.004883 * leitura;   // (5V/1024)

  double resistencia = (8000 * tensao) / (5 - tensao); // divisor de tensao

  double temperaturaK = 1 / (0.0033557 + (log(resistencia/R0))/BETA); // formula NTC
  
  return temperaturaK - 273;    // temperatura em celsius
}
