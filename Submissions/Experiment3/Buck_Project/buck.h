/*
 * buck.h
 *
 *  Created on: Feb 21, 2017
 *      Author: Vivek
 */

#ifndef BUCK_H_
#define BUCK_H_

#include "QmathLib.h"

//*********** Structure Definitions *******//
typedef struct {
	int16_t  Ref;  		// Input: reference set-point
	int16_t  Fbk;   		// Input: feedback
	int16_t  Out;   		// Output: controller output
} PI_GRANDO_IQ_TERMINALS;
// note: c1 & c2 placed here to keep structure size under 8 words

typedef struct {
	int16_t  Kp;			// Parameter: proportional loop gain
	int16_t  Ki;			// Parameter: integral gain
	int16_t  Umax;			// Parameter: upper saturation limit
	int16_t  Umin;			// Parameter: lower saturation limit
} PI_GRANDO_IQ_PARAMETERS;

typedef struct {
	int16_t  up;			// Data: proportional term
	int16_t  ui;			// Data: integral term
	int16_t  v1;			// Data: pre-saturated controller output
	int16_t  i1;			// Data: integrator storage: ui(k-1)
	int16_t  w1;			// Data: saturation record: [u(k-1) - v(k-1)]
} PI_GRANDO_IQ_DATA;

typedef struct {
	PI_GRANDO_IQ_TERMINALS term;
	PI_GRANDO_IQ_PARAMETERS param;
	PI_GRANDO_IQ_DATA	data;
} PI_GRANDO_IQ_CONTROLLER;

extern PI_GRANDO_IQ_CONTROLLER vpv_control;

#define NO_OF_SWEEP_POINTS			30
#define MPPT_STEP_TIMEOUT_COUNTS	300
#define MPPT_SWEEP_TIMEOUT_COUNTS	30000
//#define Q_NORM_MPP_START_VOLTAGE	_Q(0.62)  	//  15
//#define Q_NORM_MPP_START_VOLTAGE	_Q(0.587) 	//  14.1
#define Q_NORM_MPP_START_VOLTAGE	_Q(0.55)
#define Q_NORM_MPP_STEP_VOLTAGE		_Q(0.004)

typedef struct {
	int32_t mppt_sweep_timer;
	int32_t mppt_step_timer;
	int16_t mppt_norm_vstep;
	int16_t mppt_norm_vstart;
	int16_t mppt_power_array[NO_OF_SWEEP_POINTS];
	int8_t mppt_sweep_flag;
	int8_t mppt_step_flag;
	int8_t mppt_pow_idx;
} mppt_block_t;

extern mppt_block_t mpp_block;

//*****************************************//

#define pi_controller_macro(v, fbk)																			\
{																											\
	/* Write feedback */																					\
	v->term.Fbk = fbk;																						\
																											\
	/* proportional term */																					\
	v->data.up = _Qmpy(-(v->term.Ref - v->term.Fbk),v->param.Kp);											\
																											\
	/* integral term */																						\
	v->data.ui = _Qmpy(v->param.Ki, _Qmpy(v->data.w1, -(v->term.Ref - v->term.Fbk))) + v->data.i1;			\
	v->data.ui = _Qsat(v->data.ui, v->param.Umax, v->param.Umin);											\
	v->data.i1 = v->data.ui;																				\
																											\
	/* control output */																					\
	v->data.v1 = v->data.up + v->data.ui;																	\
	v->term.Out= _Qsat(v->data.v1, v->param.Umax, v->param.Umin);											\
	v->data.w1 = (v->term.Out == v->data.v1) ? _Q(0.9999) : _Q(0.0);										\
}

//*****************************************//

/*********** Global functions *************/
void pi_init(PI_GRANDO_IQ_CONTROLLER *pi_block);
void pi_controller(PI_GRANDO_IQ_CONTROLLER *v, int16_t fbk);
void mpp_sweep_algortihm(mppt_block_t *mpp, PI_GRANDO_IQ_CONTROLLER *vpv_pi);
void mpp_block_init(mppt_block_t *mpp);
int8_t get_max_power_idx(int16_t *power);

#endif /* BUCK_H_ */
