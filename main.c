 #include <msp430g2553.h>

/* This code utilizes an old Launch Pad Development board to send "full tank" message to Smartmi humidifier 2 motherboard 
 * in place of a dead water sensor controller. The development board can be connected to the end of the harness that provided power and TX/RX interface to the broken sensor controller. (see below)
 * 
 * Smartmi humidifier 2 sensor harness                           Launch Pad Dev Board EXP-430G2 @ <MSP430Gxxxx>
 *      _____                                                     ______________________
 * ----|    -| pin1 (5V)-----------------------------------------|- TP1  (next to USB port)
 * ----|    -| pin2 (GND)----------------------------------------|- TP3  (next to USB port)
 * ----|    -| pin3 (RX FOR WATER SENSOR)------------------------|- P1.2 (UART TX)
 * ----|    -| pin4 NC (TX)                                      |
 * ----|____-| pin5 NC                                           | CONFIG: HW UART JUMPER CONFIG, INTERNAL DCO OSCILLATOR)
 *     
 * Because the RX transmissions are cycling and not in reponse to the main unit requests It normally takes a couple of seconds before the main unit reads this communications properly and the display indicates a full tank.
 */

#define WDTp_GATE_32768    0x0000  		// watchdog source/32768
#define WDTp_GATE_8192     0x0001  		// watchdog source/8192
#define WDTp_GATE_512      0x0002  		// watchdog source/512
#define WDTp_GATE_64       0x0003  		// watchdog source/64

#define ACLK_DIV_1		   DIVA_0		// ACLK - no divider
#define ACLK_DIV_2		   DIVA_1		// ACLK/2 divider
#define ACLK_DIV_4		   DIVA_2		// ACLK/4 divider
#define ACLK_DIV_8		   DIVA_3		// ACLK/8 divider

unsigned int full_tank_msg[]={ 0x7D, 0x03, 0x6E, 0x72, 0x71, 0x03, 0x00, 0x6C, 0x4C, 0x3B, 0x03, 0x2F, 0x15, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x03, 0xE8, 0x00, 0x2C, 0x02, 0x6D, 0x37, 0xD2, 0xAD, 0xFA, 0x2A, 0x98, 0x02, 0x6D, 0x02, 0x8C, 0x05, 0xB4, 0x02, 0x05, 0x00, 0x4F, 0x00, 0x14, 0x02, 0x8C, 0x05, 0x99, 0x00, 0x06, 0x01, 0x9F, 0x02, 0x10, 0x00, 0x1E, 0x2C, 0x67, 0x02, 0xA1, 0x00, 0x0C, 0x00, 0x21, 0x01, 0x14, 0x02, 0x16, 0x02, 0x91, 0x05, 0x86, 0xB6, 0xFA, 0x29, 0x03, 0x00, 0x00, 0x00, 0x00, 0x14, 0x9A, 0x00, 0x00, 0x7D, 0x03, 0x73, 0x72, 0x71, 0x03, 0x00, 0x6C, 0x4C, 0x3B, 0x03, 0x2F, 0x15, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x21, 0x00, 0x00, 0x03, 0xE8, 0x00, 0x2C, 0x02, 0x6D, 0x37, 0xD2, 0xBF, 0xFA, 0x29, 0x03, 0x00, 0x00, 0x00, 0x00, 0x14, 0x95, 0x00, 0x00 };
// Source: https://github.com/tomasvilda/humidifierstart

void UART_send_byte(unsigned int byte);
void UART_send_msg(unsigned int message[]);
void config(void);

void config(void)
{
//USER PARAMETERS START HERE----------------------------------------------------------------------------------------------------------------------------
	#define gateWDT   WDTp_GATE_512		        // Sellects the time period between UART transmissions 
	#define aclkDIV	  ACLK_DIV_8		        // Sellects the time period between UART transmissions
    // time period is ~1/(12kHz/gateWDT/aclDIV) seconds at room temp
//USER PARAMETERS END HERE-------------------------------------------------------------------------------------------------------------------------

	WDTCTL = WDTPW + WDTHOLD;   				// Stop WDT
	//Basic Clock Module config
	BCSCTL1 = CALBC1_1MHZ; 						// Set range to 1MHz
	DCOCTL = CALDCO_1MHZ; 						// Set DCO to 1MHz
	BCSCTL3 |= LFXT1S_2;        				// LFXT1 = VLO (Slow, low power, internal oscillator)
	BCSCTL1 |= aclkDIV;							// ACLK divider

	//I/O pin registers clear
	P1DIR = 0xFF;								//Set P1 as output
	P1OUT = 0x00;								//Set P1 pins low 
	//NOTE: this order of init causes garbage on Tx line (?)
 	P1SEL = 0x00;								//Clear P1 function register
	P1SEL2 = 0x00;

	P2DIR = 0xFF;              					// Set P2 as output
	P2OUT = 0x00;								// Set P2 pins low
 	P2SEL = 0x00;								// Clear function register
	P2SEL2 = 0x00;

	//Port3 is not present on the package pins but it is present in the chip and should also be initialized to default.
	P3DIR = 0xFF;              					// Set P3 as output
	P3OUT = 0x00;								// Set P3 pins low
 	P3SEL = 0x00;								// Clear function register
	P3SEL2 = 0x00;

    //HW UART config (defaults already set: 8-bit, LSB first , One stop bit)
    P1SEL |= (BIT1 + BIT2) ; 					// P1.1 = RXD, P1.2=TXD
    P1SEL2 |= (BIT1 + BIT2) ; 					// P1.1 = RXD, P1.2=TXD
    UCA0CTL1 |= UCSSEL_2; 						// Use SMCLK as clock source
    //UCA0CTL0 &= ~UC7BIT; 						// 8-bit is default
    UCA0BR0 = 104; 								// Set baud rate to 9600 with 1MHz clock (Data Sheet 15.3.13)
    UCA0BR1 = 0; 								// Set baud rate to 9600 with 1MHz clock
    UCA0MCTL = UCBRS0; 							// Modulation UCBRSx = 1
    UCA0CTL1 &= ~UCSWRST; 						// Initialize USCI state machine
    //IE2 |= UCA0RXIE; 							// Enable USCI_A0 RX interrupt

    __delay_cycles(12000);                      // Give Smartmi main unit some time
    // Watchdog Timer config - use watchdog as regular timer (Interval Mode)
    WDTCTL = (WDTPW+WDTTMSEL+WDTSSEL+gateWDT+WDTCNTCL); 
    // 05Ah to prevent PUC, Interval Mode, Clock Source = ACLK (SMCL is reserved for UART here), Clock Divider, reset the counter, start the counter.
    
    //IE1 |= WDTIE;  // enable WDTIFG interrupt (otherwise at the end of the watchdog interval we do not trigger an interrup routine)
}

//MAIN-------------------------------------------------
int main(void)
{
	config();

	while(1)
	{
        IE1 &= ~WDTIE;                          //disable WDT interrupt

        UART_send_msg(full_tank_msg);

        IE1 |= WDTIE;                           //enable WDTIFG interrupt (otherwise at the end of the watchdog interval we do not trigger an interrup routine)

        __bis_SR_register(LPM3_bits+GIE);       //enter sleep ( LPM3 Disables SMCLK (_-_-_->|UART|) but not ACLK (_-_-_->|WATCHDOG TIMER|) )
    }
}
//------------------------------------------------------

void UART_send_msg(unsigned int message[])
{
    int i;
    for (i = 0; i < 130; i++) 
    {
        UART_send_byte(message[i]);
    }
}

//Sends one byte on the serial interface
void UART_send_byte(unsigned int byte)
{
    P1OUT ^= BIT6;					            // toggle green LED1 (P1.6) (UART busy)
	UCA0TXBUF = byte;			                // load data into USCI TX Buffer to send the byte
    while (!(IFG2 & UCA0TXIFG));	            // wait for USCI Buffer to clear - also to make sure we dont enter LPM3 with some data still in the buffer (LPM3 disables UART clock - SMCLK)
	P1OUT ^= BIT6;					            // toggle green LED1 (P1.6)
}

// This ISR triggers cycliclaly to pace the UART transmissions.
// The triggering period of this ISR can be adjusted using gateWDT and aclkDIV parameters.
#pragma vector=WDT_VECTOR
__interrupt void watchdog_timer(void)
{
	P1OUT ^= BIT0;				                // toggle red LED1 (P1.0) (WDT ISR busy)
    __delay_cycles(100);                        // make it visible to the naked eye
	P1OUT ^= BIT0;

    //NOTE: WDTIFG watchdog interrupt flag will be cleared atutomatically when finished servicing this ISR because we are in interval mode
	__bic_SR_register_on_exit(LPM3_bits);	    //do not return to LPM3 on exit from this ISR (on return SMCLK will be enabled for UART)
}
