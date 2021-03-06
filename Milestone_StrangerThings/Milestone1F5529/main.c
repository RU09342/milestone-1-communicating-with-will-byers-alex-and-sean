
#include <msp430.h>

 // Initial Greeting you should see upon properly connecting your Launchpad
int i = 0;
volatile int count = 0;
volatile int BitRX = 0;
int main(void)
{

    WDTCTL = WDTPW + WDTHOLD;                 // Stop WDT


    P3SEL = BIT3+BIT4;                        // P3.4,5 = USCI_A0 TXD/RXD
    UCA0CTL1 |= UCSWRST;                      // **Put state machine in reset**
    UCA0CTL1 |= UCSSEL_2;                     // SMCLK
    UCA0BR0 = 6;                              // 1MHz 9600 (see User's Guide)
    UCA0BR1 = 0;                              // 1MHz 9600
    UCA0MCTL = UCBRS_0 + UCBRF_13 + UCOS16;   // Modln UCBRSx=0, UCBRFx=0,
                                              // over sampling
    UCA0CTL1 &= ~UCSWRST;                     // **Initialize USCI state machine**
    UCA0IE |= UCRXIE;                         // Enable USCI_A0 RX interrupt

    //Initialize LEDs for pwm
    P1DIR |= 0x1C;  //LEDs set as output
    P1SEL |= 0x1C;  //LEDs set to TA0.1/2/3

    //Set up CCRs for clock count and pwm control
    TA0CCR0  = 0x00FF;		//After timerA counts to 255 reset
    TA0CCTL1 = OUTMOD_7;	//Put Capture Control 1 in set and reset mode
    TA0CCR1 = 0;			//Initialize Capture Control Register 1 to 0
    TA0CCTL2 = OUTMOD_7;	//Put Capture Control 2 in set and reset mode
    TA0CCR2  = 0;			//Initialize Capture Control Register 2 to 0
    TA0CCTL3 = OUTMOD_7;	//Put Capture Control 3 in set and reset mode
    TA0CCR3 = 0;			//Initialize Capture Control Register 3 to 0
    TA0CTL = TASSEL_2 + MC_1 + ID_3;	//TimerA uses SMCLK, with divider of 8, in count up mode
    __bis_SR_register(LPM0_bits + GIE);       // Enter LPM0, interrupts enabled
    __no_operation();                         // For debugger


}

// Echo back RXed character, confirm TX buffer is ready first
// Interupt vector for UART communication
#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCI_A0_VECTOR))) USCI_A0_ISR (void)
#else
#error Compiler not supported!
#endif
{
    switch(__even_in_range(UCA0IV,USCI_UCTXIFG)){

        case USCI_NONE: break;

        case USCI_UCRXIFG:
		//Switch through the count which shows which byte is being recieved
            switch (count){

                case 0:
					//While the transmit buffer is still in use wait
                    while(!(UCA0IFG & UCTXIFG));
					//Set the temporary varriable BitRx to the recived buffer
                    BitRX = UCA0RXBUF;
					//Subract 3 from the recived byte and transmit it (this is the number of byte to expect for the next module)
                    UCA0TXBUF = UCA0RXBUF - 0x0003;
                    __no_operation();
                    break;

                case 1:
					//Sets CCR1 to the 2nd byte recieved
                    TA0CCR1 = (UCA0RXBUF);
                    break;

                case 2 :
					//Sets CCR2 to the 3rd byte recieved
                    TA0CCR2 = (UCA0RXBUF);
                    break;

                case 3 :
					//Sets CCR3 to the 4th byte recived
                    TA0CCR3 = (UCA0RXBUF);
                     break;

                default:
					//If past the first 4 bytes then transmit the byte recieved once transmit buffer is cleared
                    while(!(UCA0IFG & UCTXIFG));
                    UCA0TXBUF = UCA0RXBUF;
            }
			//If not at the end of the message then increment count
            if(count != BitRX - 1){
                count += 1;
             } else if (count == BitRX - 1){
				 //At the end of the message reset count in anticipation for the next message
                 count = 0;

             }
                break;
             case USCI_UCTXIFG : break;
             default: break;
                }
            }


