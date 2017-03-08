#include "instrument.h"

#include "protocol.h"

// FIXME: this is needed for __disable_irq
#include <stm32f0xx_hal.h>

static uint32_t s_cpu_units_per_second;

// Note: we currently rely on these being zero-initialized
//static ChannelState channels[kNumChannels];
//static PulseMeasureState pulse_channels[kNumPulseChannels];

static int32_t s_freq_mode = MODE_COUNTER;
static int32_t s_aperture = 1000;

volatile uint32_t meas_rec_period, meas_rec_pulseWidth;
volatile uint32_t meas_rec_valid;

/*volatile uint32_t meas_tdelta_edge1, meas_tdelta_edge2;
volatile uint32_t meas_tdelta_period1, meas_tdelta_period2;
volatile uint32_t meas_tdelta_valid;*/

#if 0
static void UninitializeChannel(size_t index) {
}

static void UpdatePulseChannel(size_t ch) {
	static int pw = 1;

	if (HAL_GetTick() >= pulse_channels[ch].last_sent + tPulseInfoPeriod) {
		__disable_irq();
		pulse_channels[ch].last_sent = HAL_GetTick();

		DataPulse data;

		/*if (pulse_channels[ch].flags & kChannelDataValid) {
			data.timespan = pulse_channels[ch].timespan;
			data.count = pulse_channels[ch].count;
			data.period = pulse_channels[ch].period;
			data.pulse_width = pulse_channels[ch].pulse_width;
		}
		else {
			data.timespan = 0;
		}

		pulse_channels[ch].count = 0;
		pulse_channels[ch].flags &= kChannelDataValid;*/

		const int etrMode = 1;
		if (etrMode) {
			data.timespan = pulse_channels[ch].timespan;
			data.count = TIM2->CNT;
			data.period = 2;
			data.pulse_width = 1;

			TIM2->CNT = 0;
		}
		__enable_irq();

		//protocolSendPulseInfoTEST(&data);

		pw += 2;
		if (pw > 100)
			pw = 1;

		static int khz = 10;
		//if (++khz > 100)
		//	khz = 10;
		pw = 20;
		HWSetGeneratorPWM(480 / khz, 100, 100 * pw / 100);
	}
}
#endif

int instrumentGetTdelta(int* tdelta_out) {//, float* freq1_out, float* freq2_out) {
	*tdelta_out = HWGetTdelta();
	return 0;

#if 0
	if (meas_tdelta_valid/* && meas_tdelta_period1 && meas_tdelta_period2*/) {
		//*freq1_out = s_cpu_units_per_second / meas_tdelta_period1;
		//*freq2_out = s_cpu_units_per_second / meas_tdelta_period2;

		*tdelta_out = meas_tdelta_edge2 - meas_tdelta_edge1;
		meas_tdelta_valid = 0;

		return 0;
	}
	else
		return -2;
#endif
}

void instrumentSetAperture(int ms) {
	s_aperture = ms;
}

int instrumentMeasureFrequency(float* freq_out, int* duty_out) {
	HWSetFreqMode(s_freq_mode);
	meas_rec_valid = 0;

	if (s_freq_mode == MODE_COUNTER) {
		int timeframe = s_aperture;

		etr_cnt_reset();
		start_timeframe(timeframe);

		//while (!timeframe_finished) {
		//}

		int count = TIM2->CNT;
		*freq_out = (float)count / (timeframe * 0.001f);
		*duty_out = 0;
	}
	else if (s_freq_mode == MODE_RECIPROCAL) {
		// TODO: do this better
		HAL_Delay(10);

		if (meas_rec_valid && meas_rec_period != 0) {
			*freq_out = s_cpu_units_per_second / meas_rec_period;

			// Max pulse width: 2^31/100 => 447.8 ms @ 48 MHz!
			*duty_out = meas_rec_pulseWidth * 100 / meas_rec_period;
		}
		else {
			*freq_out = 0;
			*duty_out = 0;
			return -2;
		}

		meas_rec_valid = 0;
	}
	else
		return -1;

	return 0;
}

int instrumentMeasurePeriod(unsigned int* period_out, unsigned int* pulse_out) {
	instrumentSetFreqMode(MODE_RECIPROCAL);

	if (meas_rec_valid && meas_rec_period != 0) {
		*period_out = meas_rec_period;

		// Max pulse width: 2^31/100 => 447.8 ms @ 48 MHz!
		*pulse_out = meas_rec_pulseWidth;
	}
	else {
		*period_out = 0;
		*pulse_out = 0;
		return -2;
	}

	// TODO: timeout
	//meas_rec_valid = 0;
	return 0;
}

int instrumentMeasurePhaseAtoB(int* period_out, int* interval_out) {
	instrumentSetFreqMode(MODE_TDELTA);
	meas_rec_valid = 0;

	// TODO: do this better
	HAL_Delay(10);

	while (!meas_rec_valid) {
	}

	*period_out = meas_rec_period;

	if (meas_rec_pulseWidth < meas_rec_period / 2)
		*interval_out = meas_rec_pulseWidth;
	else
		*interval_out = meas_rec_pulseWidth - meas_rec_period;

	return 0;
}

void instrumentInit(uint32_t cpu_units_per_second) {
	protocolInit("Virtual Counter v1", cpu_units_per_second);
	s_cpu_units_per_second = cpu_units_per_second;
}

/*void ConfigureChannel(size_t index, const ChannelConfig* conf) {
	// undo previous settings
	UninitializeChannel(index);

	// set up GPIO direction & mode

	// set up ADC (if applicable)

	// set up channel buffer

	// set up sampling interrupt
}*/

/*void ConfigurePulseMeasurement(size_t channel, const PulseMeasureConfig* conf) {
	pulse_channels[channel].conf = *conf;

	pulse_channels[channel].count = 0;
	pulse_channels[channel].timespan = tPulseInfoPeriod;

	pulse_channels[channel].flags = 0;

	//HWSetPulseMeasurement(conf->prescaler);
	//int khz = 10;
	//HWSetGeneratorPWM(48, 100, 30);
}*/
/*
void PulseInfo(uint32_t period, uint32_t pulse_width) {
	pulse_channels[0].period = period;
	pulse_channels[0].pulse_width = pulse_width;
	pulse_channels[0].count++;

	pulse_channels[0].flags |= kChannelDataValid;
}
*/
void instrumentProcess(void) {
	/*for (size_t i = 0; i < kNumChannels; i++) {
		if (channels[i].conf.mode != kChannelModeOff) {
				//...
		}
	}

	for (size_t chan = 0; chan < kNumPulseChannels; chan++) {
		if (pulse_channels[chan].conf.mode != kPulseMeasureOff) {
			UpdatePulseChannel(chan);
		}
	}*/

	protocolProcess();
}

void instrumentSetFreqMode(int freq_mode) {
	if (s_freq_mode == freq_mode)
		return;

	s_freq_mode = freq_mode;

	HWSetFreqMode(freq_mode);

	meas_rec_valid = 0;
	//meas_tdelta_valid = 0;
}
