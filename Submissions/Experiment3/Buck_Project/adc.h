/*
 * adc.h
 *
 *  Created on: Feb 21, 2017
 *      Author: Vivek
 */

#ifndef ADC_H_
#define ADC_H_

#include <msp430.h>
#include <intrinsics.h>
#include <stdint.h>

typedef struct {
	uint16_t raw_counts;
	int16_t q15_norm_res;
	int16_t q15_norm_filtered;
}adc_result_t;

#define _Q10toQ15(A)	((A)<<5)

extern adc_result_t adc[3];



#endif /* ADC_H_ */
