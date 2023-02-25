//***********************************************************
// Zbernica I2C
//***********************************************************

#include <msp430.h>


unsigned char i,j,outLED;


void main(void)
{
    WDTCTL = WDTPW + WDTHOLD;   // zastav watchdog
    outLED=1;
    i=1;
    j=1; //adresa bola poslanaa, j=2 -daata boli poslane

    P1DIR = 1; //P1.0 vystup (zel LED)

    //nastavenie alternativnej funkcie pinov
    // Piny P1.6 and P1.7 dalej nebudu riadene bitmi registra P1OUT:

    P1SEL |= BIT6|BIT7;  //pripoj USCI SCL na pin P1.6(BIT6) a linku SDA na pin P1.7(BIT7)
                         //pin P1.5 {hodiny pri SPI) nepouzivame
    P1SEL2 |= BIT6|BIT7;     //to istee aj tu: pripoj USCI SCL na pin P1.6(BIT6) a linku SDA na pin P1.7(BIT7)

    P2DIR |= BIT5; //vystup pre modru LED
    P2OUT &= ~BIT5; //zhasni modru LED


    //inicializacia periferie I2C_USCI

    //uved modul do resetu, potom: I2C Master, zdroj hodiin,
    // adresa slave-u, smer daat, ......, uvolni USCI z resetu:

    // pred robenim zmien v USCI module, uved ho do stavu resetu:
    UCB0CTL1 |= UCSWRST; //uvedenie do resetu, pre istoru, po PUC je uz modul v resete

    //v I2C moode mame rovnako nazvany register UCB0CTL0,
    //ale jeho styri horne bity maju uplne inyy vyznam ako v SPI moode!!!
    // v pripade registra UCB0CTL1, je v nom viacej nastavovacich bitov, ktore NEboli
    // pouzite v rezime SPI


            //****** nastavenie bitov v registri UCB0CTL0 *****//

    //stav UCB0CTL0 po resete: 0x01, UCSYNC bit = 1, synchroonny mod -
    // - je riadeny hodinovym signalom

    //bity registra UCB0CTL0, ktore je potrebne nulovat (pre istotu)
    UCB0CTL0 &= ~(UCA10|UCSLA10|UCMM); // tieto bity nuluj, pre istotu
    //UCA10 - vlastna addresa UCSI je 7bit-ova
    //UCSLA10 - adresa slave-u je 7-bit-ova
    //UCMM - nie multi-master, blok porovnavania adresy je neaktivny

    //bity registra UCB0CTL0, ktore je potrebne nastavit do log.1
    UCB0CTL0 |= (UCMST|UCMODE1|UCMODE0|UCSYNC);   //bity do log.1, musia byt nastavene

    //UCMST - master mode
    //UCMODE1|UCMODE0 - I2C mode
    //UCSYNC - synchroonny rezim, pre istotu


            //****** nastavenie bitov v registri UCB0CTL1 *****//

    //obsah registra UCB0CTL1 po resete: 0x01, USCI modul je drzany v stave resetu,
    // uz bolo spominane

    //bity registra UCB0CTL1 ktore su sice nulove ale pre istotu ich nulujeme
    UCB0CTL1 &= ~(UCSSEL1|UCTXNACK|UCTXSTP|UCTXSTT); //nulovanie bitov a sucasne musi byt modul stale v resete (UCSWRST bit=1)
    //UCTXNACK - potvrdzuj normalne, ACK bit
    //UCTXSTP negeneruj STOP stav, neskuor ho pouzijeme
    //UCTXSTT negeneruj START stav, neskuor ho pouzijeme


    //bity registra UCB0CTL1, ktore je potrebne nastavit do log.1
    UCB0CTL1 |=UCSSEL0|UCSSEL1|UCTR|UCSWRST;
    // zdroj hodin SMCLK, 1MHz, (UCSSEL1,0 = 1)
    // UCTR=1 - modul USCI bude vysielat data do externeho obvodu slave
    // (po vyslani adresy slave modul vysle R/*W bit =0)
    // UCSWRST=1 - drz modul v resete, pre istotu

    //registre delicky hodinoveho signalu:
    // musia byt nastavene po PUC resete, lebo nie su automaticky inicializovane
    UCB0BR0 = 8; //delicka, spodnych 8 bitov, delenie 8, minimalne dovolene je :4
    UCB0BR1 = 0; //delicka, hornych 8 bitov,
    // signal SCL bude mat frekvenciu 1000000/8=125000Hz. nie,
    // ale 112000 Hz (merane osciloskopom)

        //***** nastavenie adresy slave-u, register UCBxI2CSA
    UCB0I2CSA=0x3A; //spravna adresa 0111010
    //UCB0I2CSA=0x4C; //nespravna adresa

        //uvolnenie modulu USCI z resetu****

    UCB0CTL1 &= ~UCSWRST; //uvolni modul USCI z resetu

    //vyber priznakov
    IFG2 &=~(0xC); //vynuluj USCI B0 RX and TX priznaky
    IE2 |= UCB0TXIE; //povol priznak po starte vysielania dat/addresy
    // jeden vektor pre prijem, UCB0TXIFG aj vysielnie, UCB0RXIFG

    UCB0STAT &= ~(UCNACKIFG|UCSTPIFG|UCSTTIFG|UCALIFG); // nuluj vsetky stavove priznaky USCI I2C
    //UCB0I2CIE |= UCNACKIE|UCSTPIE|UCSTTIE|UCALIE;// povol vsetky stavove priznaky I2C
    UCB0I2CIE |= UCNACKIE; // povol len priznak nepotvrdenia
    // USI je uvolnene davno z resetu


//nastavenie modulu casovaca a oscilator LFXT

//test start oscilatora LFXT
    P1OUT = 0x01;   // zelena LED svieti ak LFXT nebezi
    __bic_SR_register(OSCOFF);
        do {
           IFG1 &= ~OFIFG;
           __delay_cycles(50);
           } while((IFG1&OFIFG));
    P1OUT = 0x00; // zhasni LED, LFXT bezi

//nastavenie casovaca
    CCR0 = 3000;

   TACTL = TASSEL_1 + MC_2;    // ACLK je zdrojom hodin

   CCTL0 = CCIE;           // povol prerusenie, zmaz priznak

   //****** koniec nastavenie casovaca ***

   _BIS_SR(LPM0_bits + GIE);

}


#pragma vector = TIMER0_A0_VECTOR
__interrupt void porov (void)
{
// vyrob novy stav LED slpca
    switch (i)
    {
        case 1:
            outLED=outLED<<1; //zasviet LED o jednu vyssie
            if(outLED==0) // ak je 1 mimo 8 bitov
            {
                outLED=0x40; // vrat na predposlednu poziciu
                i=2;
            }
        break;
        case 2:
            outLED=outLED>>1; //zasviet LED o jednu nizsie
            if(outLED==0) // ak je 1 mimo 8 bitov
            {
                outLED=2; // vrat na predposlednu poziciu z druheho konca
                i=1;
            }
        break;
    }



    UCB0CTL1 |=UCTXSTT;   //start vysielania
    j=1; // najprv addresu slave

    CCR0 += 32768;

    P1OUT ^=1;///zmen zelenu LED

}

#pragma vector = USCIAB0TX_VECTOR   //USCI_B0 I2C vysiel/prij addr/data!!!!
__interrupt void adresa_data (void)  // spusti sa po priznakoch UCB0TXIFG a UCB0RXIFG
{
// po zacati vysielanie adresy sa prvy raz spusti aby naplnila datami TXBUF
//druhy raz sa spusti, ked sa zacnu vysielat data. Vtedy je ten spravny cas
// nastavit generovanie STOP, ktore sa ale neprejavi okamzite ale az po vyslanu daat
// do TXBUF sme v druhom behu nic nezapisali tak sa vygeruje ten STOP stav

switch (j){
case 1:
    j=2;
    UCB0TXBUF=~outLED;
    break;
case 2:
    j=1;
    UCB0CTL1 |=UCTXSTP; //az sa data vyslu, vysli stop
    break;

}

    P2OUT ^= BIT5; //zmen modru LED
    IFG2 &=~(0x0C); //nuluj priznaky USCI RX/TX adresa/data


}
#pragma vector = USCIAB0RX_VECTOR   //USCI_B0 I2C status, nie priznak CB0RXIFG!
__interrupt void status (void)  // spusti sa ak UCALIFG, UCNACKIFG, ICSTTIFG, UCSTPIFG
{
    UCB0CTL1 |=UCTXSTP; //adresa nerozpoznana vysli stop
    UCB0STAT &= ~(UCNACKIFG|UCSTPIFG|UCSTTIFG|UCALIFG); // vynuluj vsetky priznaky
    //P2OUT ^= BIT5; //zmen modru LED
}










