#include <msp430.h>
int i,j;
void main(void)
{
    WDTCTL = WDTPW + WDTHOLD;       // zastav watchdog
    // nastavenie riadiacich a datovych registrov portu P1
    P1DIR = BIT0|BIT6;              // pimy P1.0 a P1.6 nastav na vystup
    P1OUT = 0x00;                   // zhasni obe LED

    P2REN=0x3f; //povoli rezistory, aby definovali log uroven
    P2OUT=0x00; //pull down, P2DIR je nulový, vstup, to je v poriadku

    //nastavenie riadiacich registrov prerusenia od potu P1
    P1IES = BIT1|BIT2;   //reaguj na zostupnu hranu iba na P1.1,P1.2
    P1IES  &= ~ BIT3; //ak chcem nabeznu hranu....
    P1IFG = 0;      //nuluj vsetkych osem priznakov

    P1IE = BIT1|BIT2|BIT3;    //povol spustenie ISR len od pinu P1.1,P1.2,P1.3

    _BIS_SR(GIE);   //povol vsetky maskovatelne preruseni
                    //od tohto okamihu je mozne spustit ISR

    _BIS_SR(GIE + LPM4_bits);// uved CPU do nizkoprikonoveho rezimu LPM4

    while(1);       //nekonecna slucka
}
//#pragma vector = 2 // aj takto je mozne definovat vektor prerusenia od portu P1
                    // pozri v: .../ccsv5/ccs_base/msp430/include/msp430g2231.h

#pragma vector = PORT1_VECTOR //vector ktory osetruje interupt
__interrupt void nieco (void)
{
/*
    for (j=30; j > 0; j--){
            P1OUT ^= 0x01;          //zmen stav na p1.0
            for (i = 20000; i > 0; i--);    //programove oneskorenie
            }
        P1OUT = 0x00;               //vypni obe LED
    P1IFG &= ~BIT1;                //viacpriznakova ISR, mazanie prizankov programom

*/

    if (P1IFG&BIT1){
    P1OUT ^= 0x01;      //zmen len zelenu LED
    P1IFG &= ~BIT1;     //nuluj len priznak P1IFG.1
    }

    if (P1IFG&BIT2){
    P1OUT ^= 0x40;      //zmen len zelenu LED
    P1IFG &= ~BIT2;     //nuluj len priznak P1IFG.1
    }

    __delay_cycles(2000);

    if (P1IFG&BIT3){
        if(P1IN&BIT4)
        {
            P1OUT ^= 0x01;  //zmen zelenu LED
        }
        else
        {
            P1OUT ^= 0x40;  //zmen cervenu LED
        }
    P1IFG &= ~BIT3;     //nuluj len priznak P1IFG.3
    }
}
