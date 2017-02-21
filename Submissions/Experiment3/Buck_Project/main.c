#include <msp430.h>
#include <intrinsics.h>
#include <stdint.h>


/* Tasks:
 * 1. Create 10ms Timer to trigger ADC conversions
 * 2. Generate ADC interrupt to read results
 */

/* Function Prototypes */
void msp_initialise();
void SetVcoreUp (unsigned int level);
void timerA_initialise();
void port_initialise();
void msp_power_clk_init();

void timerA_initialise()
{
	// Configure TimerA0.0 as ADC conversion tirgger (period ~ 62ms)
	TA0CCR0 = 31250;                          // PWM Period
	TA0CCR1 = 31250;
	TA0CCTL1 = OUTMOD_4;                       // TA0CCR0 toggle
//	TA0CCTL0 = OUTMOD_4;                       // TA0CCR0 toggle
	TA0CTL = TASSEL__SMCLK + ID__8 + MC_1 + TACLR;          // SCLK, divide clk_src/8, up mode
}

void adc_configure()
{
	// Configure ADC10 - Pulse Sample Mode, TimerA0.0 Trigger
	ADC10CTL0 = ADC10SHT_3 + ADC10MSC + ADC10ON;         // 32 ADC10CLKs; ADC ON; Single trigger for a sequence
	ADC10CTL1 = ADC10DIV_4 + ADC10SSEL_3 + ADC10SHP + ADC10SHS_1 + ADC10CONSEQ_3; // TA0.0 trig., rpt sequence of channels
	ADC10CTL2 |= ADC10RES;                    // 10-bit conversion results
	ADC10MCTL0 = ADC10SREF_1 + ADC10INCH_8 ; // A10, internal Vref+

	// Configure internal reference
	while(REFCTL0 & REFGENBUSY);              // If ref generator busy, WAIT
	REFCTL0 |= REFVSEL_2+REFON;               // Select internal ref = 2.5V
	__delay_cycles(75);						  // Vref to settle for 75uS

	ADC10IE |= ADC10IE0;                      // Enable ADC conv complete interrupt

}

void SetVcoreUp (unsigned int level)
{
  	// Subroutine to change core voltage
    // Open PMM registers for write
    PMMCTL0_H = PMMPW_H;
    // Set SVS/SVM high side new level
    SVSMHCTL = SVSHE + SVSHRVL0 * level + SVMHE + SVSMHRRL0 * level;
    // Set SVM low side to new level
    SVSMLCTL = SVSLE + SVMLE + SVSMLRRL0 * level;
    // Wait till SVM is settled
    while ((PMMIFG & SVSMLDLYIFG) == 0);
    // Clear already set flags
    PMMIFG &= ~(SVMLVLRIFG + SVMLIFG);
    // Set VCore to new level
    PMMCTL0_L = PMMCOREV0 * level;
    // Wait till new level reached
    if ((PMMIFG & SVMLIFG))
      while ((PMMIFG & SVMLVLRIFG) == 0);
    // Set SVS/SVM low side to new level
    SVSMLCTL = SVSLE + SVSLRVL0 * level + SVMLE + SVSMLRRL0 * level;
    // Lock PMM registers for write access
    PMMCTL0_H = 0x00;
}

void msp_power_clk_init()
{
	__delay_cycles(500000);

	// Increase Vcore setting to level3 to support fsystem=25MHz
    // NOTE: Change core voltage one level at a time..
    SetVcoreUp (0x01);
    SetVcoreUp (0x02);
    SetVcoreUp (0x03);

    // Initialize DCO to 25MHz
    __bis_SR_register(SCG0);                  // Disable the FLL control loop
    UCSCTL0 = 0x0000;                         // Set lowest possible DCOx, MODx
    UCSCTL1 = DCORSEL_6;                      // Select DCO range 4.6MHz-88MHz operation
    UCSCTL2 = FLLD_1 + 763;                   // Set DCO Multiplier for 25MHz
                                              // (N + 1) * FLLRef = Fdco
                                              // (762 + 1) * 32768 = 25MHz
                                              // Set FLL Div = fDCOCLK/2
    __bic_SR_register(SCG0);                  // Enable the FLL control loop

    // Worst-case settling time for the DCO when the DCO range bits have been
    // changed is n x 32 x 32 x f_MCLK / f_FLL_reference. See UCS chapter in 5xx
    // User Guide for optimization.
    // 32 x 32 x 25 MHz / 32,768 Hz = 782000 = MCLK cycles for DCO to settle
    __delay_cycles(782000);
}

void port_initialise()
{
    P1DIR |= 0x01;				// Set P1.0 to output direction (to drive LED)
    P1OUT |= 0x01;				// Set P1.0  - turn LED on
}

void msp_initialise()
{
	port_initialise();
	msp_power_clk_init();
    adc_configure();
	timerA_initialise();
//	timerD_initialise();
}

void main(void)
{
//	unsigned int FirstADCVal;

    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
    msp_initialise();

    ADC10CTL0 |= ADC10ENC;

    __bis_SR_register(GIE);        // Enter LPM3 w/ interrupt

    while(1) {}
}

uint16_t data;
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR (void)
{
  switch(__even_in_range(ADC10IV,12))
  {
    case  0: break;                          // No interrupt
    case  2: break;                          // conversion result overflow
    case  4: break;                          // conversion time overflow
    case  6: break;                          // ADC10HI
    case  8: break;                          // ADC10LO
    case 10: break;                          // ADC10IN
    case 12:
		P1OUT ^= 0x01;               // Set P1.0 LED on
		ADC10IFG = 0;
		data = ADC10MEM0;
    default: break;
  }
}
