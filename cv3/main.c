//Mikropocitace a ich programovanie
//******************************************************************************
// Priklad nastavenia modulu generovania hodinovych signalov
//******************************************************************************

#include <msp430.h>

short int i;

void blink(char n, unsigned int cykly);
void delay(unsigned int j);

void main(void)
{
    WDTCTL = WDTPW + WDTHOLD; // zastavenie casovaca watchdog

    P1DIR |= 0x41;            //nastav piny portu P1.0 and P1.6 ako vystupne,
                              //funkcia ostatnych pinov portu sa nemeni

    P1OUT = 0x01;             // log.1 na P1.0 a log.0 na vsetky ostatne piny,
                              //teda aj na P1.6

// Kalibracia oscilatora DCO na DCO_FREQ = 1MHz a jeho zapnutie

    BCSCTL1 = CALBC1_1MHZ;
    DCOCTL = CALDCO_1MHZ;
    __bic_SR_register(SCG0);    //zapnutie DCO
                                //intrizicka funkcia


// Test rozbehu krystalom riadeneho oscilatora LFXT1OSC

    __bic_SR_register(OSCOFF); // zapnutie LFXT1 , pre istotu
    do {
       IFG1 &= ~OFIFG;
       __delay_cycles(50);
       } while((IFG1&OFIFG)); // cakanie na rozbeh LFXT1


while(1){

//PRVE nastvenie zdroja hodinoveho signalu DCO->MCLK

        BCSCTL3 &= ~(LFXT1S1|LFXT1S0);  //prepnutie medzi VLO (10) a LFXT1 (00)
                                        //prepnutie na LFXT1
        BCSCTL2 &= ~(SELM1|SELM0);      //medzi DCOCLK (00/01) a VLOorLFXT1 (10/11) pre MCLK
                                        // DCOCLK -> MCLK
        BCSCTL2 &= ~(DIVM1|DIVM0);      // delicka MCLK 1:1



        blink(5, 60000); // 10-krat preblikni LED - cerv-zel-cerv-zel-cerv-zel-cerv-zel-cerv-zel
                            //cislo 60000 nevyjadruje cas ale pocet vykonani funkcie "void delay(int j);"

        delay(60000);   // svit LED sa chvilu nemeni, (ostava svietit zelena)
        delay(60000);

// DRUHE nastavenie zdroja hodinoveho signalu (DCO/2)->MCLK

        BCSCTL2 |= (DIVM0);             // delicka MCLK 1:2
        blink(5, 30000);
        delay(30000);
        delay(30000);
        IFG1 &= ~OFIFG;                 //musíme nastavit na jednotku

// TRETIE nastavenie zdroja hodinoveho signalu LFXT1->MCLK
// nastudujte spravanie sa oscilatorov VLO a LFXT pri ich prepínaní
// MSP430x2xxFamUsersGuideB.pdf strana 277 a 280

        BCSCTL2 |= SELM1;               //prepneme VLOorLFXT1 (10/11)
        BCSCTL2 &= ~(DIVM1|DIVM0);      //delicka MCLK 1:1
        blink(5, 1966);
        delay(1966);                    // 60000*(frekvencia)/1000000
        delay(1966);

// STVRTE nastavenie zdroja hodinoveho signalu VLO->MCLK

        BCSCTL3 |= LFXT1S1;             //prepneme VLO (10)
        BCSCTL3 &= ~LFXT1S0;            //prepneme LFXT1 (00)
        blink(5,720);

        delay(720);
        delay(720);

        P1OUT = 0x00;                   //vypneme led
        delay(720);
        delay(720);
        P1OUT = 0x01;                   //zapneme led P1.0
    }
}

void blink(char n, unsigned int cykly)
{
    for(i=0;i<n;i++){
        delay(cykly);
        P1OUT ^= 0x41;  //bitova operacia exclusive OR.
                        //0x41=0b01000001, instrukcia precita stav vsetkych pinov portu P1
                        //nacitane hodnoty bit po bite X-OR -uje s 0x41, vysledok zapise na vsetky piny portu
                        //co svietilo - zhasne, co bolo zhasnute zasvieti. Ale len na bitovej pozicii .0 a .6
                        //teda tam, kde operand 0x41 obsahuje log. 1
        delay(cykly);
        P1OUT ^= 0x41;
}

}

void delay(unsigned int j)
{
    do {(j--);
    asm(" nop");    //funkcia musi obsahovat nejaku zmysluplnu instrukciu - staci aj assemblerovska " nop"
                    //inak prekladac funkciu "void delay(long int j)" vobec neprelozi a program ju nebude
                    //nikdy volat
    }
    while (j != 0);
}
