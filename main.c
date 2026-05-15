#include <18F4550.h>
#device ADC = 10
#fuses INTRC, PLL1, CPUDIV1, NOWDT, NOPROTECT, NOLVP, NOMCLR

#use delay(clock = 1000000)

#include "registries.h"

int8 prescalerIndex = 0;

const int8 prescalerBits[3] = {
    0b00, // 1:1BB
    0b01, // 1:4
    0b10  // 1:16
};

void setPWM1Duty10(int16 duty) {
  int16 maxDuty = (((int16)PR2 + 1) * 4) - 1;
  if (duty > maxDuty) {
    duty = maxDuty;
  }

  CCPR1L = duty >> 2;

  CCP1CON = (CCP1CON & 0b11001111) | ((duty & 0b11) << 4);
}

void adcSetup(void) {
  TRISA0 = 1;
  ADCON1 = 0x0E;
  ADCON2 = 0b10010001;
  ADCON0 = 0b00000001;

  delay_us(50);
}

int16 readADC0(void) {
  int16 result;

  ADCON0 = (ADCON0 & 0b11000011) | 0b00000000;

  delay_us(20);

  ADONE = 1;

  while (ADONE) {
  }

  result = ((int16)ADRESH << 8) | ADRESL;

  return result;
}

void ccpSetup(void) {
  PR2 = 4; // Frecuencia PWM de aproximadamente 50 kHz (con Fosc = 1 MHz)
  //^ Cambiado para 50 kHz

  CCP1CON = 0x8C;
  setPWM1Duty10((((int16)PR2 + 1) * 4) / 2); // Ciclo de trabajo inicial del 50%
}

void actualizarT2CON(void) {
  TMR2ON = 0;
  T2CON = 0x04 | prescalerBits[prescalerIndex];

  TMR2 = 0;
  TMR2IF = 0;
}

void enableExternalInterrupts(void) {
  TRISB0 = 1;
  INT0IF = 0;
  INT0IE = 1;
  INTEDG0 = 1;

  GIE = 1;
}

#INT_EXT
void externalInterrupt(void) {
  LATB2 = !LATB2;

  prescalerIndex++;

  if (prescalerIndex >= 3) {
    prescalerIndex = 0;
  }

  actualizarT2CON();
}

void main(void) {
  OSCCON = 0x42;

  adcSetup();

  TRISC2 = 0;
  TRISB2 = 0;
  TRISD5 = 0;

  LATB2 = 0;

  ccpSetup();

  prescalerIndex = 0;
  actualizarT2CON();

  enableExternalInterrupts();

  while (TRUE) {
    int16 adcValue;

    adcValue = readADC0();
    if (adcValue > (((int16)PR2 + 1) * 4 - 1)) {
      adcValue = (((int16)PR2 + 1) * 4 - 1);
    }
    //^ Esto es para evitar que el valor del ADC exceda el máximo permitido por el periodo actual del PWM, causando que truene.

    setPWM1Duty10(adcValue);

    delay_ms(10);
  }
}