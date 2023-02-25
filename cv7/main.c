//***********************************************************
// Stopky s LED displejom a riaden�m, 4x 7segmentov
//***********************************************************

#include <msp430.h>

#define FS_H        P1OUT |= BIT0; //(1 << 0)
#define FS_L        P1OUT &= ~BIT0; //(1 << 0)

void fs_strobe(void);


void fs_strobe(void) //vygeneruj prepisovac� pulz na P1.0
    {
        FS_H; //P1OUT |= BIT0;
        asm("   nop");
        FS_L; //P1OUT &= ~BIT0;
    }



const unsigned char tvary_cislic[10] = {0x3F, 6, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7F, 0x6F};
// ulozene do FLASH
// //"1"     6 = 0000 0110, hgfedCBa
// //"6"  0x7c = 0111 1100 -> 0111 1101 0x7d
// //"0"  0x3F = 0011 1111, hgFEDCBA

unsigned char jed_sek=0, des_sek=0, jed_min=0, des_min=0, RXin;

unsigned char i=0; //ktor� r�d sa vysiela


void main(void)
{

    WDTCTL = WDTPW + WDTHOLD;               // zastav watchdog

    i=1;


    P1OUT = 0x00;             // inicializ�cia P1, hlavne kv�li prepisovaciemu pulzu
    P1DIR |= BIT0;  // nastav P1.0 na v�stup, smer ostatn�ch pinov sa nastav� pri pripojovan� perif�rie USCI ni��ie

    //nastavenie alternat�vnej funkcie pinov. Piny P1.5 a P1.7 u� nebud� vic riaden� registrom P1OUT:

    P1SEL |= BIT7|BIT5;       //pripoj UCB0MOSI na pin P1.7 (BIT7) a UCB0CLK na P1.5 (BIT5)
                            //sign�l UCB0MISO nepou��vame, tak pin P1.6 ostane v z�kladnej I/O funkcii, cez P1OUT
    P1SEL2 |= BIT7|BIT5;     //to ist�: pripoj UCB0MOSI na pin P1.7 (BIT7) a UCB0CLK na P1.5 (BIT5)


    // nastavenie modulu/periferie USCI pre mod SPI...
     // nez budeme menit nastavenie mudulu USCI je ho potrebne uviest do resetu:
     UCB0CTL1 |= UCSWRST; //vstup do stavu reset, po resete procesora je uz v resete, len pre istotu


     //stav UCB0CTL0 reg. po PUC : 0x01, UCSYNC bit = 1, synchronny mod
     UCB0CTL0 &= ~(UCCKPH|UCCKPL|UC7BIT|UCMODE0|UCMODE1); // bity do log.0, pre istotu
     UCB0CTL0 |= (UCCKPH|UCMSB|UCMST|UCSYNC);   //bity do log.1, MUSIME nastavit

     //zdroj signalu pre UCBOCLK:
    // UCB0CTL1 &= ~(UCSSEL1); //bity do nuly,
     //UCB0CTL1 |=UCSSEL0; //bity do log.1, zdroj hodin je ACLK, 32kHz

     //UCB0CTL1 &= ~(UCSSEL1); //bity do nuly,
     UCB0CTL1 |=(UCSSEL0|UCSSEL1); //bity do log.1, zdroj hodin je ACLK, 32kHz

     //delicka frekvencie UCB0CLK - dva registre:
     // registre nie su inicializovane po resete, treba ich nastavit
     UCB0BR0 = 1; //delicka hodin, spodnych 8 bitov, delenie 1
     UCB0BR1 = 0; //delicka hodin, hornych 8 bitov

     UCB0CTL1 &= ~UCSWRST; //uvolni USCI z resetu, modul je pripraveny vysielat data

     // ******************** koniec nastavovania modulu USCI
     // teraz je modul USCI v m�de SPI a je pripraven� vysiela� d�ta

    IE2 |= UCB0RXIE; //UCB0TXIE; //povol priznak od PRIJATIA dat, nie od zaciatku vysielania
     // pokial som nezacal vysielat priznak UCB0RXIFG sa nema ako nastavit a spustit ISR
    //nastavenie riadiacich registrov prerusenia od potu P1
        P1IES = BIT1|BIT2;   //reaguj na zostupnu hranu iba na P1.1
        //P1IES  &= ~ BIT3; //ak chcem nabeznu hranu....
        P1IFG = 0;      //nuluj vsetkych osem priznakov

        P1IE = BIT1|BIT2|BIT3;    //povol spustenie ISR len od pinu P1.1



//kontrola �innosti kry�t�lov�ho oscil�tora LFXT, je potrebny ako pre casovac tak aj pre USCI

    P1OUT = 0x40;   // zelena led indikuje, ze LFXT este nebezi
    __bic_SR_register(OSCOFF);      // zapni oscilator LFXT1, len pre istoru
        do {
           IFG1 &= ~OFIFG;      //periodicky nunluj priznak nefunkcnosti oscilatora
           __delay_cycles(50);
           } while((IFG1&OFIFG));   //je stale nefunkcny?
    P1OUT = 0x00; // LFXT bezi, zhasni LED

//nastavenie casovaca - urcuje dlzku pauzy medzi jednotlivymi vysielaniami bloku 4x8 bitov dat z USCI

    CCR0 = 3000;               // komparacny register CCR0 (TACCR0)
                               //startovacia hodnota
   TACTL = TASSEL_1 + MC_2;    // hodinovy signal pre casovac bude ACLK, 32kHz

// ************** koniec nastavovania modulu casovaca


   CCTL0 = CCIE;           // povol spustenie ISR ked dojde k rovnosti pocitadla TAR a registra CCR0
                           //prikaz tiez sucasne nuluje priznak TACCTL0_CCIFG

   _BIS_SR(LPM0_bits + GIE);    // vstup do modu LPM0, povolenie vsetkych maskovatelnych preruseni
                                // LPM0 - vypni len MCLK, oscilator LFXT1CLK MUSI bezat stale,
                                //  lebo napaja casovac aj seriovy port

    }

    // ISR od �asova�a, LEN od pr�znaku TACCTL0_CCIFG

#pragma vector = TIMER0_A0_VECTOR
__interrupt void komp0 (void)
{
  // ISR sa spustila lebo bol obsah registrov TAR a CCR0 rovnak� (teraz u� nemus� tak by�)

    jed_sek++;
    //jed_sek--;


    if (jed_sek==10){jed_sek=0,des_sek++;}
    if (des_sek==6){des_sek=0,jed_min++;}
    if (jed_min==10){jed_min=0,des_min++;}
    if (des_min==6){des_min=0;}

    if (jed_sek==255){jed_sek=9,des_sek--;}
    if (des_sek==255){des_sek=5,jed_min--;}
    if (jed_min==255){jed_min=9,des_min--;}
    if (des_min==255){des_min=5;}



    UCB0TXBUF =  ~(tvary_cislic[jed_sek]);
    //do registra vlo� prvok z po�a pod�a hodnoty ��slice r�dy jedotiek sek�nd
    //�tart prenosu prv�ch 8 bitov
    i=1; // ktor� r�d bude poslan� ako nasleduj�ci - des_sek

    CCR0 += 32768; //spusti ISR od �asova�a ka�d� sekundu

}

#pragma vector = USCIAB0RX_VECTOR   //bude in� start after data have been sent
__interrupt void after_sent (void)
{
    switch(i)
            {
            case 1:
                UCB0TXBUF =  ~(tvary_cislic[des_sek]);
                i=2; // ktor� r�d bude poslan� �al�� - jed_min)
            break;

            case 2:
                UCB0TXBUF =  ~(tvary_cislic[jed_min]);
                i=3; // ktor� r�d bude poslan� �al�� - des_min)
            break;

            case 3:
                UCB0TXBUF =  ~(tvary_cislic[des_min]);
                i=4; // �o bude poslan� ako �al�ie  - prepisovac� pulz
            break;

            case 4:
                fs_strobe(); //prepisovac� pulz

                i=5;    // ak i=5 ziadny case neplati len zmaz priznak
            break;
            }

    RXin=UCB0RXBUF; // precitanim UCB0RXBUF nulujem priznak UCB0RXIFG

}

#pragma vector = PORT1_VECTOR
__interrupt void nieco (void)
{

    if (P1IFG&BIT1){

    CCTL0 ^= CCIE;

    P1IFG &= ~BIT1;     //nuluj len priznak P1IFG.1
    }

    if (P1IFG&BIT2){

    jed_sek=0, des_sek=0, jed_min=0, des_min=0;

    UCB0TXBUF =  ~(tvary_cislic[jed_sek]);
    i=1; // ktor� r�d bude poslan� ako nasleduj�ci - des_sek


    P1IFG &= ~BIT2;     //nuluj len priznak P1IFG.1
    }



    if (P1IFG&BIT3){
        if(P1IN&BIT4)
        {
        jed_sek++;

        }
        else
        {
        jed_sek--;

        }
    P1IFG &= ~BIT3;     //nuluj len priznak P1IFG.3

    if (jed_sek==10){jed_sek=0,des_sek++;}
       if (des_sek==6){des_sek=0,jed_min++;}
       if (jed_min==10){jed_min=0,des_min++;}
       if (des_min==6){des_min=0;}

       if (jed_sek==255){jed_sek=9,des_sek--;}
       if (des_sek==255){des_sek=5,jed_min--;}
       if (jed_min==255){jed_min=9,des_min--;}
       if (des_min==255){des_min=5;}

       UCB0TXBUF =  ~(tvary_cislic[jed_sek]);
       i=1; // ktor� r�d bude poslan� ako nasleduj�ci - des_sek

    }

}









