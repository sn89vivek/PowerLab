#include <msp430.h>

void SetVcoreUp (unsigned int level);

/*
 * main.c
 */
void main(void) {
    volatile unsigned long i;	// Declare counter variable
    WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer

    P1SEL |= BIT6;				// Set P1.6 to output direction (Timer D0.0 output)
    P1DIR |= BIT6;
    P1SEL |= BIT7;				// Set P1.7 to output direction (Timer D0.1 output)
    P1DIR |= BIT7;
    P2SEL |= BIT0;				// Set P2.0 to output direction (Timer D0.2 output)
    P2DIR |= BIT0;
    P1DIR |= 0x01;				// Set P1.0 to output direction (to drive LED)
    P1OUT |= 0x01;				// Set P1.0  - turn LED on
    __delay_cycles(500000);
    P1OUT ^= 0x01;				// Toggle P1.0 using exclusive-or function  - turn LED off

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

     // Configure TimerD in Hi-Res Regulated Mode
    TD0CTL0 = TDSSEL_2;                       // TDCLK=SMCLK=25MHz=Hi-Res input clk select
    TD0CTL1 |= TDCLKM_1;                      // Select Hi-res local clock
    TD0HCTL1 |= TDHCLKCR;					  // High-res clock input >15MHz
    TD0HCTL0 = TDHM_0 + 					  // Hi-res clock 8x TDCLK = 200MHz
    		   TDHREGEN + 					  // Regulated mode, locked to input clock
    		   TDHEN;     					  // Hi-res enable

    // Wait some, allow hi-res clock to lock
    P1OUT ^= 0x01;							  // Toggle P1.0 using exclusive-OR, turn LED on
    __delay_cycles(500000);
    while(!TDHLKIFG);					      // Wait until hi-res clock is locked
    P1OUT ^= 0x01;							  // Toggle P1.0 using exclusive-OR, turn LED off

    // Configure the CCRx blocks
    TD0CCR0 = 2000;                           // PWM Period. So sw freq = 200MHz/2000 = 100 kHz
    TD0CCTL1 = OUTMOD_7 + CLLD_1;             // CCR1 reset/set
    TD0CCR1 = 1000;                           // CCR1 PWM duty cycle of 1000/2000 = 50%
    TD0CCTL2 = OUTMOD_7 + CLLD_1;             // CCR2 reset/set
    TD0CCR2 = 500;                            // CCR2 PWM duty cycle of 500/2000 = 25%
    TD0CTL0 |= MC_1 + TDCLR;                  // up-mode, clear TDR, Start timer

    for (;;) {						// Infinite loop, blink LED
     		P1OUT ^= 0x01;			// Toggle P1.0 output
     		i = 1000000;
     		do(i--);
     		while(i != 0);			// Wait 10000 cycles
     	}
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

uint16_t vbat,ibat;
float duty = 0.5; /* initial converter duty */
void main()
{
	initialise_msp();
	while()
	{
		adc_results_get(&vbat, &ibat);
		if(vbat > BATTERY_FLOAT)
		{
			/* Battery fully charged. Turn off converter */
			duty = 0;
		}
		else
		{
			mppt_algo(vbat, ibat);
		}
		converter_duty_set(duty);
	}
}

void converter_duty_set(float d)
{
	TD0CCR1 = (uint16_t)(duty*(float)TD0CCR0);
}

void mppt_algo(uint16_t vb, uint16_t ib)
{
	static uint32_t power_prev = 0;
	static float duty_step = 0.05;
	uint32_t power_new;

	power_new = vb*ib;

	if(power_new > power_prev)
	{
		duty = duty + duty_step
	}
	else
	{
		duty_step = -duty_step;
		duty = duty + duty_step;
	}

	/* Limits on duty cycle */
	if(duty < 0)
	{
		duty = 0;
	}
	else if(duty > 0.98)
	{
		duty = 0.98
	}
	power_prev = power_new;
}

uint16_t volt_counts, current_counts;
uint32_t power_last = 0;
float D = 0;
float duty_step = 0.05;
void get_max_point()
{
	uint32_t power_this;

	power_this = volt_counts * current_counts;

	if(power_this > power_last)
	{
		D += duty_step;
	}
	else
	{
		D = -duty_step;
		D += duty_step;
	}
}
