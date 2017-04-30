/*
 * buck.c
 *
 *  Created on: Feb 21, 2017
 *      Author: Vivek
 */

#include "buck.h"
#include "adc.h"

PI_GRANDO_IQ_CONTROLLER vpv_control;
mppt_block_t mpp_block;

/************ PI init *****************************/
//*********** Structure Init Function ****//
void pi_init(PI_GRANDO_IQ_CONTROLLER *pi_block)
{
	// m = 0.75; c = 0.137
	// act_ref = cal_ref*m + c
	// cal_ref = 1.2*Vref/11.2/2.5
//	pi_block->term.Ref=_Q(0.6830);
//	pi_block->term.Ref=_Q(0.715);
	pi_block->term.Ref= Q_NORM_MPP_START_VOLTAGE;
	pi_block->term.Fbk=0;
	pi_block->term.Out=0;

	pi_block->param.Kp = _Q(0.1);
	pi_block->param.Ki = _Q(0.1);
	pi_block->param.Umax = _Q(0.98);
	pi_block->param.Umin = _Q(0.1);

	pi_block->data.up = _Q(0.0);
	pi_block->data.ui = _Q(0.0);
	pi_block->data.v1 = _Q(0.0);
	pi_block->data.i1 = _Q(0.0);
	pi_block->data.w1 = _Q(0.9999);
}

/*********** PI controller ************************/
void pi_controller(PI_GRANDO_IQ_CONTROLLER *v, int16_t fbk)
{
	/* Write feedback */
	v->term.Fbk = fbk;

	/* proportional term */
	v->data.up = _Qmpy(-(v->term.Ref - v->term.Fbk),v->param.Kp);

	/* integral term */
	v->data.ui = _Qmpy(v->param.Ki, _Qmpy(v->data.w1, -(v->term.Ref - v->term.Fbk))) + v->data.i1;
	v->data.ui = _Qsat(v->data.ui, v->param.Umax, v->param.Umin);
	v->data.i1 = v->data.ui;

	/* control output */
	v->data.v1 = v->data.up + v->data.ui;
	v->term.Out= _Qsat(v->data.v1, v->param.Umax, v->param.Umin);
	v->data.w1 = (v->term.Out == v->data.v1) ? _Q(0.9999) : _Q(0.0);
}

void mpp_block_init(mppt_block_t *mpp)
{
	mpp->mppt_step_timer = MPPT_STEP_TIMEOUT_COUNTS;
	mpp->mppt_sweep_timer = MPPT_SWEEP_TIMEOUT_COUNTS;
	mpp->mppt_norm_vstart = Q_NORM_MPP_START_VOLTAGE;
	mpp->mppt_norm_vstep = Q_NORM_MPP_STEP_VOLTAGE;
	mpp->mppt_pow_idx = 0;
	mpp->mppt_sweep_flag = 0;
	mpp->mppt_step_flag = 0;
}

void mpp_sweep_algortihm(mppt_block_t *mpp, PI_GRANDO_IQ_CONTROLLER *vpv_pi)
{
	int8_t max_idx;
	int16_t vset;
	if(mpp->mppt_sweep_flag == 1)
	{
		/* Set ref to vstart at start of sweep */
		if(mpp->mppt_pow_idx == 0)
			vpv_pi->term.Ref = mpp->mppt_norm_vstart;

		/* End of a step */
		if(mpp->mppt_step_flag == 1)
		{
			/* Capture power at end of every step */
			mpp->mppt_power_array[mpp->mppt_pow_idx] = _Qmpy(adc[0].q15_norm_filtered,adc[1].q15_norm_filtered);

			/* Increment vpv step */
			vpv_pi->term.Ref += mpp->mppt_norm_vstep;

			/* Power array Index increment */
			mpp->mppt_pow_idx++;

			mpp->mppt_step_flag = 0;
		}

		/* End of sweep?? */
		if(mpp->mppt_pow_idx == NO_OF_SWEEP_POINTS)
		{
			max_idx = get_max_power_idx(mpp->mppt_power_array);
			vset = mpp->mppt_norm_vstart;
			while(max_idx > 0)
			{
				vset += mpp->mppt_norm_vstep;
				max_idx--;
			}
			vpv_pi->term.Ref = vset;
			mpp->mppt_sweep_flag = 0;
			mpp->mppt_pow_idx = 0;
		}
	}
}

int8_t get_max_power_idx(int16_t *power)
{
	int8_t max_idx, i;
	max_idx = 0;
	for(i = 0; i < NO_OF_SWEEP_POINTS; i++)
		if(power[i] >= power[max_idx])
			max_idx = i;
	return max_idx;
}
