#include "virtualinstrument/hw.h"
#include "virtualinstrument/instrument.h"

#include "../../../common/protocoldefs.h"

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

void instrumentReset(void) {
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

int instrumentFinishMeasurePulseCount(unsigned int* count_out) {
	if (s_instrument_state != STATE_MEASURING || s_measurement_state.mode != MEASUREMENT_PULSE_COUNT)
		return -1;

	if (!HWGetCounterValue(count_out))
		return 0;

	s_instrument_state = STATE_READY;
	return 1;
}

int instrumentStartMeasurePeriod(unsigned int iterations) {
    if (s_instrument_state != STATE_READY)
        return -1;

    if (HWInitPeriodMeasurement(iterations) <= 0)
        return -1;

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

int instrumentFinishMeasurePeriod(uint64_t* period_out, uint64_t* pulse_width_out) {
    if (s_instrument_state != STATE_MEASURING || s_measurement_state.mode != MEASUREMENT_PERIOD)
        return -1;

    if (!HWGetPeriodPulseWidth(period_out, pulse_width_out))
        return 0;

    s_instrument_state = STATE_READY;
    return 1;
}

int instrumentFinishMeasurePhaseShift(unsigned int* period_out, int* interval_out) {
    if (s_instrument_state != STATE_MEASURING || s_measurement_state.mode != MEASUREMENT_PHASE)
        return -1;

    uint64_t period, pulse_width;

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

int instrumentStartMeasureFreqRatio(unsigned int iterations) {
    return -1;
}

int instrumentFinishMeasureFreqRatio(unsigned int* ratio_out) {
    return -1;
}
