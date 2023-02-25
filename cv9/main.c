

//**************** externy D/A prevodnik, vzorky z tabulky, ********* Direct Digital Synthesis
#include <msp430.h>

const unsigned int vzorky[] = {2046,2247,2445,2640,2829,3010,3183,3344,3493,3628,3747,3850,3936
        ,4004,4053,4082,4092,4082,4053,4004,3936,3850,3747,3628,3493,3344,3183,3010,2829,2640,
        2445,2247,2046,1845,1647,1452,1263,1082,909,748,599,464,345,242,156,88,39,10,0,10,39,88,
        156,242,345,464,599,748,909,1082,1263,1452,1647,1845,2046,2247 //};
        ,2445,2640,2829,3010,3183,3344,3493,3628,3747,3850,3936};

        //,4004,4053,4082,4092,4082,4053,4004,3936,3850,3747,3628,3493,3344,3183,3010,2829,2640 };
        //,2445,2247,2046,1845,1647,1452,1263,1082,909,748,599,464,345,242,156,88,39,10,0,10,39,88
    //  ,156,242,345,464,599,748,909,1082,1263,1452,1647,1845};

//pole vzorky je ulozene v pamati FLASH, lebo ma modifikator const


unsigned int i=0, j=0, k=16 ;
char horne8, dolne8, RXin;


#define tlv5636_cntrl   0x9001      //pomale ustalenie vyst. napatia,3252,  referencia 1.024V

void main(void)
{

    WDTCTL = WDTPW + WDTHOLD;
// inicializacia P1

    P1OUT = 0x00;
    P1DIR |= BIT6|BIT1;  // pin P1.6 bude generova pulz FS

//nastavenie alternativnej funkcie pinov.
//Piny P1.5 and P1.7 uz dalej nebudu riadene registrom P1OUT:

    P1SEL |= BIT7|BIT5;       //pripoj vyvod USCI UCB0MOSI (bude  vystup dat) na pin P1.7 (BIT7)
                              //a vystup hodin. sig. UCB0CLK na pin P1.5 (BIT5)
                            //vyvod MISO, P1.6 nebudeme pouzivat, ostane riadeny reg-om P1OUT
    P1SEL2 |= BIT7|BIT5;     //to iste, MOSI na P1.7, UCBOCLK na P1.5


// nastavenie modulu/periferie USCI pre mod SPI...

    // nez budeme menit nastavenie mudulu USCI je ho potrebne uviest do resetu:
    UCB0CTL1 |= UCSWRST; //vstup do stavu reset, po resete procesora je uz v resete, len pre istotu


    //stav UCB0CTL0 reg. po PUC : 0x01, UCSYNC bit = 1, synchronny mod
    UCB0CTL0 &= ~(UCCKPH|UCCKPL|UC7BIT|UCMODE0|UCMODE1); // bity do log.0, pre istotu
    UCB0CTL0 |= (UCCKPH|UCCKPL|UCMSB|UCMST|UCSYNC);   //bity do log.1, MUSIME nastavit
    // ked bolo UCCKPH a sucasne UCCKPL v log.1, t.j. CLK bol medzi vysielaniami v log.0 a
    //data sa citaju s dobeznou hranou, tak prevodnik akceptoval kazdu druhu vzorku (16bitovu)

    //zdroj signalu pre UCBOCLK:
    UCB0CTL1 |=UCSSEL0|UCSSEL1; //bity do log.1, zdroj hodin je SMCLK, 1MHz (cca)

    //delicka frekvencie UCB0CLK - dva registre:
    // registre nie su inicializovane po resete, treba ich nastavit
    UCB0BR0 = 4; //delicka hodin, spodnych 8 bitov, delenie 4, staci rychlost takato, 250kHz
    UCB0BR1 = 0; //delicka hodin, hornych 8 bitov
    // teda UCBCLK bude  SMCLK/4 = 250kHz

    UCB0CTL1 &= ~UCSWRST; //uvolni USCI z resetu, modul je pripraveny vysielat data

// ******************** koniec nastavovania modulu USCI


    //IE2 |= UCB0RXIE; //UCB0TXIE; //povol priznak od PRIJATIA dat, nie od zaciatku vysielania
    // pokial som nezacal vysielat priznak UCB0RXIFG sa nema ako nastavit a spustit ISR
    // nic sa nepovoluje

//nastavenie riadiacich registrov prerusenia od potu P1
    P1IFG = 0;      //nuluj vsetkych osem priznakov
    P1IE = BIT3;    //povol spustenie ISR len od pinu P1.3

    //nastavenie casovaca - urcuje dlzku pauzy medzi jednotlivymi vysielaniami dat z USCI
    CCR0 = 3000;               // komparacny register CCR0 (TACCR0)
                               //startovacia hodnota

   TACTL = TASSEL_1 + MC_2;    // hodinovy signal pre casovac bude ACLK, 32kHz
                               // MC_2 - pocitaj dookola od 0 to 0xffffh a znova od 0
                               // v prikaze je "=" vsetky ostatne bity su nulovane
                               // prikaz tiez sucasne nuluje priznak TAIFG

   // ************** koniec nastavovania modulu casovaca

   CCTL0 = CCIE;           // povol spustenie ISR ked dojde k rovnosti pocitadla TAR a registra CCR0
                           //prikaz tiez sucasne nuluje priznak TACCTL0_CCIFG

// koniec nastavovani


   //naplnenie pomocnych premennych na vysielanie
    horne8 = tlv5636_cntrl>>8;  // hornych 8 bitov
    dolne8 = (char)tlv5636_cntrl; // len spodnych 8 bitov

    _BIS_SR(GIE + LPM0_bits); // vstup do hibernacie LPM0

}


// ISR od casovaca, priznak TACCTL0_CCIFG
#pragma vector = TIMER0_A0_VECTOR
__interrupt void rovnost (void)
{
    P1OUT |= BIT1; // ako dlho bezi ISR, start merania

    P1OUT |=BIT6;   //vytvor pulz na P1.6 pre signal FS prevodnika
                __delay_cycles(5);

    P1OUT &= ~BIT6;

    UCB0TXBUF = horne8;  //do shift registra vloz
                         //hornych 8 bitov konfiguracneho slova
                         //prevodnika DAC
    UCB0TXBUF = dolne8;  // a hned aj dolne8; horne8 sa uz skopírovali do vysielacieho registra a BUF je už vo¾ný pre dolne8
                        // to je krasa buffrovaneho serioveho portu
    //i++;
    //if(i==64)i=0;

    j+=k;   //zvecsi j o fazu k, citanie kazdej vzorky pola raz -> k=16
    i=j;    //drz v j celu hodnotu pre nasledovne pricitanie hodnoty k
    i=i>>4; //odrez spodne bity, adresuj pole vyssimi bitmi i
    j&=0x3FF;//max hodnota 0x3f=63

    //vzorka = vzorky[i]; // pomalsie
    //horne8 = vzorka>>8;
    //dolne8 = (char)vzorka;

    horne8 = vzorky[i]>>8;      // vzorka z pola, hornych 8bitov
    dolne8 = (char)vzorky[i];   // vzorka z pola, spodnych 8bitov

    CCR0 += 5;// dlzka pauzy medzi dvoma vysielaniami. musi to byt dlhsie ako trva vypocet v ISR

    RXin=UCB0RXBUF; // precitanim UCB0RXBUF nulujem priznak UCB0RXIFG, ani to teraz netreba, len zo slusnosti
    P1OUT &= ~BIT1; // ako dlho trva rutina, koniec merania

}



#pragma vector = PORT1_VECTOR
__interrupt void nieco (void)
{


    if (P1IFG&BIT3){
        if(P1IN&BIT4)
        {
           k++;
           if(k>200)k=200;
        }
        else
        {
            k--;
            if(k<1)k=1;
        }
    P1IFG &= ~BIT3;     //nuluj len priznak P1IFG.3
    }


}

