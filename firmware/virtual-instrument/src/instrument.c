#include "virtualinstrument/hw.h"
#include "virtualinstrument/instrument.h"

#include "../../../common/protocoldefs.h"

static uint32_t s_cpu_units_per_second;

static int s_instrument_state;

static int s_pwm_phase;

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

    if (HWStartPulseCountMeasurement(gate_time) <= 0)
        return -1;

    s_instrument_state = STATE_MEASURING;
    s_measurement_state.mode = MEASUREMENT_PULSE_COUNT;
    s_measurement_state.gate_time = gate_time;
	return 1;
}

int instrumentFinishMeasurePulseCount(uint32_t* count_out) {
	if (s_instrument_state != STATE_MEASURING || s_measurement_state.mode != MEASUREMENT_PULSE_COUNT)
		return -1;

	if (!HWPollPulseCountMeasurement(count_out))
		return 0;

	s_instrument_state = STATE_READY;
	return 1;
}

int instrumentStartMeasurePeriod(uint32_t iterations) {
    if (s_instrument_state != STATE_READY)
        return -1;

    if (HWStartPeriodMeasurement(iterations) <= 0)
        return -1;

    s_instrument_state = STATE_MEASURING;
    s_measurement_state.mode = MEASUREMENT_PERIOD;
    return 1;
}

int instrumentStartMeasurePhaseShift() {
    if (s_instrument_state != STATE_READY)
        return -1;

    if (HWStartIntervalMeasurement() <= 0)
        return -1;

    s_instrument_state = STATE_MEASURING;
    s_measurement_state.mode = MEASUREMENT_PHASE;
    return 1;
}

int instrumentFinishMeasurePeriod(uint64_t* period_out, uint64_t* pulse_width_out) {
    if (s_instrument_state != STATE_MEASURING || s_measurement_state.mode != MEASUREMENT_PERIOD)
        return -1;

    if (!HWPollPeriodMeasurement(period_out, pulse_width_out))
        return 0;

    s_instrument_state = STATE_READY;
    return 1;
}

int instrumentFinishMeasurePhaseShift(uint32_t* period_out, int32_t* interval_out) {
    if (s_instrument_state != STATE_MEASURING || s_measurement_state.mode != MEASUREMENT_PHASE)
        return -1;

    uint32_t period, pulse_width;

    if (HWPollIntervalMeasurement(&period, &pulse_width) <= 0)
        return 0;

    *period_out = period;

    if (pulse_width < period / 2)
        *interval_out = pulse_width;
    else
        *interval_out = pulse_width - period;


    s_instrument_state = STATE_READY;
    return 1;
}

int instrumentStartMeasureFreqRatio(uint32_t iterations) {
    if (s_instrument_state != STATE_READY)
            return -1;

    if (HWStartFreqRatioMeasurement(iterations) <= 0)
        return -1;

    s_instrument_state = STATE_MEASURING;
    s_measurement_state.mode = MEASUREMENT_FREQ_RATIO;
    return 1;
}

int instrumentFinishMeasureFreqRatio(uint64_t* ratio_out) {
    if (s_instrument_state != STATE_MEASURING || s_measurement_state.mode != MEASUREMENT_FREQ_RATIO)
        return -1;

    if (HWPollFreqRatioMeasurement(ratio_out) <= 0)
        return 0;

    s_instrument_state = STATE_READY;
    return 1;
}

int instrumentSetPwm(size_t index, uint32_t period, uint32_t pulse_width, uint32_t phase) {
    unsigned int prescaler = 1;
    unsigned int prescaled = period;

    while (prescaled >= 65535) {
        prescaler++;
        prescaled = period / prescaler;
    }

    if (index == 1)
        s_pwm_phase = phase;

    if (HWSetPwm(index, prescaler, prescaled, pulse_width / prescaler, phase) <= 0)
        return -1;

    if (HWSetPwmPhase(s_pwm_phase) <= 0)
        return -1;

    return 0;
}
