#include "hw.h"
#include "instrument.h"

#include "../../Common/protocoldefs.h"

static uint32_t s_cpu_units_per_second;

static int s_instrument_state;

static struct {
	int mode;
	int gate_time;
} s_measurement_state;

void instrumentInit(uint32_t cpu_units_per_second) {
	s_cpu_units_per_second = cpu_units_per_second;
	s_instrument_state = STATE_READY;
}

void instrumentProcess(void) {
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

    if (!meas_rec_valid)
        return 0;

    *period_out = meas_rec_period;

    // FIXME: Max pulse width: 2^31/100 => 447.8 ms @ 48 MHz!
    *pulse_width_out = meas_rec_pulseWidth;

    s_instrument_state = STATE_READY;
    return 1;
}

int instrumentFinishMeasurePhaseShift(unsigned int* period_out, int* interval_out) {
    if (s_instrument_state != STATE_MEASURING || s_measurement_state.mode != MEASUREMENT_PHASE)
        return -1;

    if (!meas_rec_valid)
        return 0;

    *period_out = meas_rec_period;

    if (meas_rec_pulseWidth < meas_rec_period / 2)
        *interval_out = meas_rec_pulseWidth;
    else
        *interval_out = meas_rec_pulseWidth - meas_rec_period;

    s_instrument_state = STATE_READY;
    return 1;
}
