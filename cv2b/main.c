/*
 * Mikropocitace a ich programovanie
 * cvicenie c. 2 softvérový test tlaèítka na porte
*/

#include  <msp430.h>

void onesk(unsigned int i); //deklaracia funkcie onesk
                            //Oneskorenie preto aby máme dostatočný
                            //čas medzi stlačením a pustením

void main(void)
{

  WDTCTL = WDTPW + WDTHOLD;  // Zastav casovac watchdog-u
  P1DIR |= 0x41;             // Nastav piny P1.0 and P1.6 do vystupneho modu
  P1DIR &= 0xF7;             // Nastav pin P1.3 do vstupnej funkcie
  unsigned int j = 0;        //kolko bitovy je short integer? 16 bity, 2 byte

  for (;;)                   //nekonecny cyklus
  {
    P1OUT = 0x00;            //vypneme P1.0 a P1.6

    while(P1IN & BIT3);     //test pustene tl           1
                            //stlacil som
    P1OUT = 0x40;           //zapneme P1.6 a vypneme P1.0
    onesk(2);
    while(~P1IN & BIT3);    //test stlacene tl
                            //pustil som
                            //led stale svieti
    onesk(2);

    while(P1IN & BIT3);     //test pustene tl           2
                            //stlacil som
    P1OUT = 0x01;           //zapneme P1.0 a vypneme P1.6
    onesk(2);
    while(~P1IN & BIT3);    //test stlacene tl
                            //pustil som
                            //led stale svieti
    onesk(2);

    while(P1IN & BIT3);     //test pustene tl           3
                            //stlacil som
    P1OUT = 0x00;           //vypneme P1.0 a vypneme P1.6
    onesk(2);
    while(~P1IN & BIT3);    // test stlacene
                            //pustil som
    P1OUT = 0x40;           //zapneme P1.6 a vypneme P1.0
    onesk(2);

    while(P1IN & BIT3);    //test pustene tl           4
                           //stlacil som
    for(j=0;j<3;j++)
        {
            P1OUT = 0x40;  //zapneme P1.6 a vypneme P1.0
            onesk(50000);
            P1OUT = 0x01; //zapneme P1.0 a vypneme P1.6
            onesk(50000);
        }
    P1OUT= 0x00;          //vypneme P1.0 a vypneme P1.6
    onesk(2);
    while(~P1IN & BIT3);  //test stlacene
                          //pustil som
    onesk(2);


    while(P1IN & BIT3);   //test pustene tl             5
                          //stlacil som
    P1OUT = 0x40;         //zapneme P1.6 a vypneme P1.0

    onesk(2);             //nasledujuci test spusti až pochvili, preco?
    while(~P1IN & BIT3);  //test stlacene tl
                          //pustil som
                          //led stale svieti
    onesk(2);
  }
}

// Cakacia funkcia
void onesk(unsigned int i)   //definicia funkcie onesk. No operation
{
    do {(i--);
    asm(" nop");            //musí by 1 medzera pred nop aby to chápal ako inštrukciu a nie ako návestie
                            //bez toho nefungoval program
    }
    while (i != 0);
}
