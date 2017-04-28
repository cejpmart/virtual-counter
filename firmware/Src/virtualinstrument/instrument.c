#include "hw.h"
#include "instrument.h"

#include "../../common/protocoldefs.h"

static uint32_t s_cpu_units_per_second;

static int s_instrument_state;

static struct {
	int mode;
	int gate_time;

	unsigned int period_sum;
	unsigned int pulse_width_sum;
	unsigned int iterations_total;
	unsigned int iterations_remaining;
} s_measurement_state;

void instrumentInit(uint32_t cpu_units_per_second) {
	s_cpu_units_per_second = cpu_units_per_second;
	s_instrument_state = STATE_READY;
}

void instrumentProcess(void) {
    if (s_instrument_state == STATE_MEASURING) {
        if (s_measurement_state.mode == MEASUREMENT_PERIOD && s_measurement_state.iterations_remaining) {
            unsigned int period, pulse_width;

            if (HWGetPeriodPulseWidth(&period, &pulse_width)) {
                s_measurement_state.period_sum += period;
                s_measurement_state.pulse_width_sum += pulse_width;
                s_measurement_state.iterations_remaining--;
            }
        }
    }
}

int instrumentStartMeasurePulseCount(int gate_time) {
    if (s_instrument_state != STATE_READY)
        return -1;

    HWSetFreqMode(MODE_COUNTER, 0);

    s_measurement_state.gate_time = gate_time;

	HWClearPulseCounter();
	HWStartTimeframe(gate_time);

	s_instrument_state = STATE_MEASURING;
	s_measurement_state.mode = MEASUREMENT_PULSE_COUNT;
	return 1;
}

int instrumentFinishMeasurePulseCount(unsigned int* freq_out) {
	if (s_instrument_state != STATE_MEASURING || s_measurement_state.mode != MEASUREMENT_PULSE_COUNT)
		return -1;

	if (!HWTimeframeElapsed())
		return 0;

	int count = TIM2->CNT;
	*freq_out = count * 1000 / s_measurement_state.gate_time;

	s_instrument_state = STATE_READY;
	return 1;
}

int instrumentStartMeasurePeriod(unsigned int iterations) {
    if (s_instrument_state != STATE_READY)
        return -1;

    HWSetFreqMode(MODE_RECIPROCAL, 0);
    HWClearPeriodMeasurement();

    s_instrument_state = STATE_MEASURING;
    s_measurement_state.mode = MEASUREMENT_PERIOD;

    s_measurement_state.period_sum = 0;
    s_measurement_state.pulse_width_sum = 0;
    s_measurement_state.iterations_total = iterations;
    s_measurement_state.iterations_remaining = iterations;
    return 1;
}

int instrumentStartMeasurePhaseShift() {
    if (s_instrument_state != STATE_READY)
        return -1;

    HWSetFreqMode(MODE_TDELTA, 0);
    HWClearPeriodMeasurement();

    s_instrument_state = STATE_MEASURING;
    s_measurement_state.mode = MEASUREMENT_PHASE;
    return 1;
}

int instrumentFinishMeasurePeriod(unsigned int* period_out, unsigned int* pulse_width_out) {
    if (s_instrument_state != STATE_MEASURING || s_measurement_state.mode != MEASUREMENT_PERIOD)
        return -1;

    if (s_measurement_state.iterations_remaining)
        return 0;

    // FIXME: Max pulse width: 2^31/100 => 447.8 ms @ 48 MHz!

    *period_out = s_measurement_state.period_sum / s_measurement_state.iterations_total;
    *pulse_width_out = s_measurement_state.pulse_width_sum / s_measurement_state.iterations_total;

    s_instrument_state = STATE_READY;
    return 1;
}

int instrumentFinishMeasurePhaseShift(unsigned int* period_out, int* interval_out) {
    if (s_instrument_state != STATE_MEASURING || s_measurement_state.mode != MEASUREMENT_PHASE)
        return -1;

    uint32_t period, pulse_width;

    if (!HWGetPeriodPulseWidth(&period, &pulse_width))
        return 0;

    *period_out = period;

    if (pulse_width < period / 2)
        *interval_out = pulse_width;
    else
        *interval_out = pulse_width - period;


    s_instrument_state = STATE_READY;
    return 1;
}
