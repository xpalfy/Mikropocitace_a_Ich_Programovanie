//******************************************************************************
// MPP Cv. 8
// Analogovo/digitalny prevodnik - voltmeter
//******************************************************************************

#include <msp430.h>

#define FS_H        P1OUT |= BIT6;
#define FS_L        P1OUT &= ~BIT6;
void fs_strobe(void);

unsigned char jed=4,des=3,sto=2,tis=1;
const unsigned char tvary_cislic[11] = {0x3F, 6, 0x5b, 0x4f, 0x66, 0x6D,
                                        0x7D, 0x07, 0x7F, 0x6F,0x40};
signed int pomoc=0;
unsigned long int vysledok;
//long int vysledok=70;
//long int vysledok1;

unsigned char i;    // ktora cislicovka sa bude vysielat

unsigned char RXin; //na kopirovanie z UCB0RXBUF

void fs_strobe(void) //Generuj SW zapisovaci pulz 74HC595, signal STCP, pin12
{
        FS_H; //P1OUT |= BIT6;
        asm(" nop");
        FS_L; //P1OUT &= ~BIT6;
}

void main(void)
{
    WDTCTL = WDTPW + WDTHOLD;       // zastav WDT

    // inicializaci P1
    P1OUT = 0x00;
    P1DIR |= BIT6;  // nastav pin 6 do vystupu, prepisovaci pulz, pin 6 nebude riadeny periferiou UCSI

    //nastavenie alternativnej funkcie pinov.
    //Piny P1.5 and P1.7 uz dalej nebudu riadene registrom P1OUT:

    P1SEL |= BIT7|BIT5;       //pripoj vyvod USCI UCB0MOSI (bude  vystup dat) na pin P1.7 (BIT7)
                              //a vystup hodin. sig. UCB0CLK na pin P1.5 (BIT5)
                            //vyvod MISO, P1.6 nebudeme pouzivat, ostane riadeny reg-om P1OUT, bude generovat prepisovaci pulz
    P1SEL2 |= BIT7|BIT5;     //to iste, MOSI na P1.7, UCBOCLK na P1.5


    // nastavenie modulu/periferie USCI pre mod SPI...
    // nez budeme menit nastavenie mudulu USCI je ho potrebne uviest do resetu:
    UCB0CTL1 |= UCSWRST; //vstup do stavu reset, po resete procesora je uz v resete, len pre istotu


    //stav UCB0CTL0 reg. po PUC : 0x01, UCSYNC bit = 1, synchronny mod
    UCB0CTL0 &= ~(UCCKPH|UCCKPL|UC7BIT|UCMODE0|UCMODE1); // bity do log.0, pre istotu
    UCB0CTL0 |= (UCCKPH|UCMSB|UCMST|UCSYNC);   //bity do log.1, MUSIME nastavit

    //zdroj signalu pre UCBOCLK:
    UCB0CTL1 &= ~(UCSSEL1); //bity do nuly,
    UCB0CTL1 |=UCSSEL0; //bity do log.1, zdroj hodin je ACLK, 32kHz

    //delicka frekvencie UCB0CLK - dva registre:
    // registre nie su inicializovane po resete, treba ich nastavit
    UCB0BR0 = 1; //delicka hodin, spodnych 8 bitov, delenie 1
    UCB0BR1 = 0; //delicka hodin, hornych 8 bitov

    UCB0CTL1 &= ~UCSWRST; //uvolni USCI z resetu, modul je pripraveny vysielat data

    // ******************** koniec nastavovania modulu USCI

    IE2 |= UCB0RXIE; //UCB0TXIE; //povol priznak od PRIJATIA dat, nie od zaciatku vysielania
    // pokial som nezacal vysielat priznak UCB0RXIFG sa nema ako nastavit a spustit ISR

//*********************   nastavenie modulu casovaca

    //najprv start oscilatora LFXT, je potrebny ako pre casovac tak aj pre USCI
        P1OUT = 0x40;   // zelena led indikuje, ze LFXT este nebezi
        __bic_SR_register(OSCOFF);      // zapni oscilator LFXT1, len pre istoru
            do {
               IFG1 &= ~OFIFG;      //periodicky nunluj priznak nefunkcnosti oscilatora
               __delay_cycles(50);
               } while((IFG1&OFIFG));   //je stale nefunkcny?
        P1OUT = 0x00; // LFXT bezi, zhasni LED

        //nastavenie casovaca - urcuje dlzku pauzy medzi jednotlivymi prevodmi
        CCR0 = 3000;               // komparacny register CCR0 (TACCR0)
                                   //startovacia hodnota

       TACTL = TASSEL_1 + MC_2;    // hodinovy signal pre casovac bude ACLK
                                   // MC_2 - pocitaj dookola od 0 to 0xffffh a znova od 0
                                   // v prikaze je "=" vsetky ostatne bity su nulovane
                                   // prikaz tiez sucasne nuluje priznak TAIFG

// ************** koniec nastavovania modulu casovaca

       CCTL0 = CCIE;           // povol spustenie ISR ked dojde k rovnosti pocitadla TAR a registra CCR0
                               //prikaz tiez sucasne nuluje priznak TACCTL0_CCIFG


       /*************** zaciatok nastavovania periferie prevodnika ADC10**********/
           ADC10CTL0 = SREF_1 + ADC10SHT_3 + ADC10ON + ADC10IE + REFON;
           // ADC10SHT_3 je interval ustalenia vstup. napatia, znaci
               // * 64 x period hodin ADC10CLK/8, tj 409.6 us, t.j. 0.4ms
           // ADC10ON - zapnutie prevodnika,
           // ADC10IE - povolenie prerusenia
           // SREF_1 -> VR+ = VREF+ and VR- = GND, treba zapnut Vref
           // REFON=1 zapni interny zdroj referencneho napatia
           // REF2_5V = 0 napatie Vref bude 1.5 V
           // MSC = 0 po skonceni prevodu sa nespusti hned novy prevod
           // ADC10IFG = 0 nulovanie priznaku,  ENC = 0 prevod zakazany

           ADC10CTL1 = INCH_10|ADC10DIV_7;         // vstup prevodnika pripojime na kanal A10,
                                                   // ktory je intrne spojeny s teplotnym cidlom

           //SHSx = 0x00 - spustenie prevodu od bitu ADC10SC
           //ADC10DF =0  - hodnota napatia vyjadrena vo formate: priamy kod (straight binary code)
           //ADC10DIV_7 = 0x00 - delicka hodin signalu 1:8 (5MHz/8=625kHz)
           //ADC10SSELx = 0x00 - volba hodinoveho signalu pre prevodnik -  oscilator ADC10OSC
       /************************koniec nastavovanie prevodnika ADC10 ********************/

    _BIS_SR(LPM0_bits + GIE);    // vstup do modu LPM0, povolenie vsetkych maskovatelnych preruseni
                                 // LPM0 - vypni len MCLK, oscilator LFXT1CLK MUSI bezat stale,
                                 //  lebo napaja casovac aj seriovy port

}


#pragma vector = TIMER0_A0_VECTOR       // spusti sa raz za sekundu
__interrupt void komp0 (void)
{
    ADC10CTL0 |= ENC + ADC10SC;         // povolenie prevodu - bit ENC a
                                        // start prevodu - bit ADC10SC

     CCR0 += 32768; //raz za sekundu

}


#pragma vector=ADC10_VECTOR         // prevod sa skoncil, bit ADC10IFG od prevodnika ADC10 je v log.1,
__interrupt void ADC10_ISR(void)
{

        //vysledok=(float)ADC10MEM*3.4931506;//konstantu vypocitat!
        vysledok=ADC10MEM * 169014;
        vysledok=vysledok - 113764761;
        vysledok = vysledok >> 12;



        /************** prevod bin do BCD ******************/
        //pomoc=vysledok;
        pomoc=(int)vysledok;    //pri odcitavani moze byt "pomoc" aj zaporna
                                ///musi byt preto definovana ako znamienkova
        jed=0;des=0,sto=0,tis=0;
        do {pomoc-=1000;
        tis++;}
            while(pomoc>=0);    // kolko je tisicok
        tis--;
        pomoc+=1000;

        do {pomoc-=100;
        sto++;}
            while(pomoc>=0);    // kolko je stoviek
        sto--;
        pomoc+=100;

        do {pomoc-=10;
        des++;}
            while(pomoc>=0);
        des--;
        pomoc+=10;

        do {pomoc-=1;
        jed++;}
            while(pomoc>=0);
        jed--;
        //pomoc+=1;

        if(tis>9){tis=10;sto=10;des=10;jed=10;} //cislo vacsie ako 9999
        //vypise pomlcky
        //potlacenie prvej nuly??

/****************** vysielanie dat na cislicovky****************/

        UCB0TXBUF = ~(tvary_cislic[jed]);   //kopíruj dáta do registra a start vysielaniav

        i=1;    // nasledujucich 8 bitov bude raad desiatok

}


#pragma vector = USCIAB0RX_VECTOR   //spusti potom, co si vyslal data (8 bitov)
__interrupt void dalsie_cislicovky (void)
{
    switch(i)
            {
            case 1:
                UCB0TXBUF =  ~(tvary_cislic[des]);
                i=2; // ktorý rád bude poslaný ïalší - stovky
            break;

            case 2:
                UCB0TXBUF =  ~(tvary_cislic[sto]|0x80); //desatinna bodka
                i=3; // ktorý rád bude poslaný ïalší - tisicky
            break;

            case 3:
                UCB0TXBUF =  ~(tvary_cislic[tis]);
                i=4; // èo bude poslané ako ïalšie  - prepisovací pulz
            break;

            case 4:
                fs_strobe(); //prepisovací pulz

                i=5;    // ak i=5 ziadny case neplati len nuluj priznak
            break;
            }

    RXin=UCB0RXBUF; // precitanim UCB0RXBUF nulujem priznak UCB0RXIFG

}

