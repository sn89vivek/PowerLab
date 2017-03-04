/*
 * adc.c
 *
 *  Created on: Feb 21, 2017
 *      Author: Vivek
 */

#include "adc.h"
#include "buck.h"

adc_result_t adc[3] = {{0,0,0},{0,0,0},{0,0,0}};
uint16_t index = 0;

#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR (void)
{
	ADC10CTL0 &= ~ADC10ENC;
	adc[index].raw_counts = ADC10MEM0;

	/* convert to normalised form */
	adc[index].q15_norm_res = _Q10toQ15(adc[index].raw_counts);

	/* exponential moving average
	 * (Link: http://dsp.stackexchange.com/questions/20333/how-to-implement-a-moving-average-in-c-without-a-buffer)
	 */
	adc[index].q15_norm_filtered = adc[index].q15_norm_filtered + ((adc[index].q15_norm_res - adc[index].q15_norm_filtered)>>3);

	index++;
	if(index == 3)
	{
		/* we come here once every 10ms */
		index = 0;
		ADC10CTL0 |= ADC10ENC;
		pi_controller_macro((&vpv_control),adc[2].q15_norm_filtered);
		TD0CCR1 = _Qmpy(vpv_control.term.Out, TD0CCR0);

		/* mppt sweep timer */
		mpp_block.mppt_sweep_timer--;
		if(mpp_block.mppt_sweep_timer == 0)
		{
			/* mpp sweep timer expired. Set sweep flag */
			mpp_block.mppt_sweep_flag = 1;
			mpp_block.mppt_sweep_timer = MPPT_SWEEP_TIMEOUT_COUNTS;
		}

		/* mppt step timer */
		if(mpp_block.mppt_sweep_flag == 1)
		{
			/* Step timer is active only when sweep is true */
			mpp_block.mppt_step_timer--;
			if(mpp_block.mppt_step_timer == 0)
			{
				/* mpp step timer expired. Set step flag */
				mpp_block.mppt_step_flag = 1;
				mpp_block.mppt_step_timer = MPPT_STEP_TIMEOUT_COUNTS;
			}
		}
	}
}


