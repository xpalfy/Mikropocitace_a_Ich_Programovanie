//**************************************************************************
// MPP cv.5
// Timer0_A3, prepinaj P1.0 a nezavisle aj P1.6, CCR0, kontinualny mod pocitadla, LFXT do SMCLK
//priorita spustenia viacerych ISR
//**************************************************************************

#include <msp430.h>


void main(void)
{
    WDTCTL = WDTPW + WDTHOLD;       // zastav WDT

    P1DIR |= 0x41;                  // piny P1.0 a P1.6 nastav ako vystupy
    P1OUT = 0x00;                   // vynuluj vsetky bity, piny portu P1
    P1IES = BIT1|BIT2;
    P1IFG = 0;
    P1IE = BIT1|BIT2;

    //Blok generovania hodinovych signalov
    //po resete je nastavene:

    //DCOCLK/1 -> MCLK -> CPU, (frekvencia cca 1000000 Hz)
    //LFXT/1   -> ACLK        (frekvencia presne 32768 Hz)

    // potrebujeme zmenit v module hodinoveho signalu:
    //LFXT/1 -> SMCLK
    BCSCTL2 |= SELS;

    // mozne nastavenie kondenzatorov oscilatora  LFXT
    //BCSCTL3 |=XCAP1; //kondenzatory pri krystali: xcap=10b -> 10 pF
    //BCSCTL3 &=~XCAP0; // po resete xcap=01b -> 6 pF, bolo malo

    P1OUT = 0x41;                   // indikaacia, ze LFXT nebezi, program ostáva v nasledovnej sluèke
    __bic_SR_register(OSCOFF);      // zapnutie oscillatora LFXT1
        do {                        // pre istotu
           IFG1 &= ~OFIFG;
           __delay_cycles(50);
           } while((IFG1&OFIFG));   // cakanie na rozbeh LFXT
    P1OUT = 0x00;                   // LFXT bezi, zhasni LED-ky

    // potom samotne nastavenie:


    //nastavenie registrov casovaca  T_A
    // pocitadlo TAR po resete nepocita, je neaktivne

    CCR0 = 32768;                   // komparacny register CCR0
    CCR1 = 16384;                   //startovacia hodnota, od nej frekvencia
                                    // blikania LED nezavisi.


    TACTL = TASSEL_2 + MC_2;        // skontrolovat parametre podla zadania, skomentovat v zavere
                                    // "Open declaration" pre TASSEL_1
        // ? je hodinovy signal SMCLK zdrojom impulzov pre pocitadlo TAR ?,
                                    // ? je start pocitania pocitadla v mode kontinualneho pocitania
                                    // (od  0 do 0xffff)?
                                    //  prikaz nuluje zvysne bity registra. Skontrolujem, co sa este
                                    //vsetko zmeni a ci si to mozem dovolit zmenit
                                    // kde je mazanie priznaku TAIFG?


    CCTL0 = CCIFG;                  //nastavenie priznaku priznaku od komparatora 0
    CCTL0 = CCIE;
    CCTL1 = CCIFG;                  //nastavenie priznaku priznaku od komparatora 1
    CCTL1 = CCIE;                   // povolenie prerusenia od priznaku nastatia rovnosti obsahu TAR a CCR0
                                    // mozem ostatne bity vynulovat? akyy bude priznak?

    _BIS_SR(LPM0_bits + GIE);       // prechod do modu LPM0, vsetky maskovatelne prer. povolit
                                    // vypneme len MCLK, LFXT oscilator musi ostat bezat, lebo napaaja casovac

}



// Obsluzny program casovaca T_A0 // priznak TACCTL0_CCIFG

#pragma vector = TIMER0_A0_VECTOR        //skontrolujte nazov vektora
__interrupt void komp0 (void)
{

    P1OUT ^= 0x01;                      // zmen log. stav na P1.0


    CCR0 += 32768;                      // pripocitaj hodnotu delta k obsahu CCR0
                                        // frekvencia zavisi od hodnoty delta
}

#pragma vector = TIMER0_A1_VECTOR
__interrupt void komp1 (void)
{

    P1OUT ^= 0x40;                      // zmen log. stav na P1.6


    CCR1 += 16384;                      // pripocitaj hodnotu delta k obsahu CCR0
    CCTL1 &= ~CCIFG;                    // frekvencia zavisi od hodnoty delta
}


#pragma vector = PORT1_VECTOR //vector ktory osetruje interupt
__interrupt void nieco (void)
{
    if (P1IFG&BIT1){
            CCTL0 ^= CCIE;
            P1OUT &= ~0x01;     //zmen len zelenu LED
            P1IFG &= ~BIT1;     //nuluj len priznak P1IFG.1
       }

       if (P1IFG&BIT2){
           CCTL1 ^= CCIE;
           P1OUT &= ~0x40;      //zmen len zelenu LED
           P1IFG &= ~BIT2;     //nuluj len priznak P1IFG.1
       }
}
