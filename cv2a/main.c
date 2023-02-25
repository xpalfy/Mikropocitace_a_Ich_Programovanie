/*
 * Mikropocitace a ich programovanie
 * cvicenie c. 2, blikac v jazyku C
*/

#include  <msp430.h>



void main(void)
{
  WDTCTL = WDTPW + WDTHOLD;  // Zastavenie watchdogu
  P1DIR |= 0x01;             // Nastavenie pinu portu P1.0 ako vystup,
                             //tam je zelena LED

     unsigned int i;          // 16 bitovy

  while(1)
  {
    P1OUT ^= 0x01;  // EX-OR, skrateny zapis, povodne:  //P1OUT = P1OUT^0x01;
    i = 50000;

    do {(i--);
    asm(" nop");
    }
  while (i != 0);
  }
}
