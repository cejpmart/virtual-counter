#include "virtualinstrument/hw.h"
#include "virtualinstrument/instrument.h"

#include "../../../common/protocoldefs.h"

static int s_instrument_state;

static int s_pwm_phase;

static struct {
	int mode;
	int gate_time;
} s_measurement_state;

void instrumentInit(void) {
	s_instrument_state = STATE_READY;
}

void instrumentReset(void) {
    // TODO: should actually abort the measurement

    s_instrument_state = STATE_READY;
}

void instrumentProcess(void) {
}

int instrumentAbortMeasurement(int mode) {
    if (s_instrument_state == STATE_MEASURING && s_measurement_state.mode == mode) {
        // TODO: should actually abort the measurement

        s_instrument_state = STATE_READY;
        return 0;
    }

    return -1;
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

int instrumentFinishMeasurePeriod(uint64_t* period_out, uint64_t* pulse_width_out) {
    if (s_instrument_state == STATE_MEASURING && s_measurement_state.mode == MEASUREMENT_PERIOD) {
        if (!HWPollPeriodMeasurement(period_out))
            return 0;

        *pulse_width_out = 0;
        s_instrument_state = STATE_READY;
        return 1;
    }
    else if (s_instrument_state == STATE_MEASURING && s_measurement_state.mode == MEASUREMENT_PWM) {
        if (!HWPollPwmMeasurement(period_out, pulse_width_out))
            return 0;

        s_instrument_state = STATE_READY;
        return 1;
    }
    else
        return -1;
}

int instrumentFinishMeasurePulseCount(uint32_t* count_out) {
	if (s_instrument_state != STATE_MEASURING || s_measurement_state.mode != MEASUREMENT_PULSE_COUNT)
		return -1;

	if (!HWPollPulseCountMeasurement(count_out))
		return 0;

	s_instrument_state = STATE_READY;
	return 1;
}

int instrumentStartMeasurePeriod(uint32_t num_periods, int with_pulse_width) {
    if (s_instrument_state != STATE_READY)
        return -1;

    if (num_periods == 0)
        return -1;

    if (!with_pulse_width) {
        if (HWStartPeriodMeasurement(num_periods) <= 0)
            return -1;

        s_instrument_state = STATE_MEASURING;
        s_measurement_state.mode = MEASUREMENT_PERIOD;
    }
    else {
        if (HWStartPwmMeasurement(num_periods) <= 0)
            return -1;

        s_instrument_state = STATE_MEASURING;
        s_measurement_state.mode = MEASUREMENT_PWM;
    }

    return 1;
}

int instrumentStartMeasureInterval(int ch1_falling, int ch2_falling) {
    if (s_instrument_state != STATE_READY)
        return -1;

    if (HWStartIntervalMeasurement(ch1_falling, ch2_falling) <= 0)
        return -1;

    s_instrument_state = STATE_MEASURING;
    s_measurement_state.mode = MEASUREMENT_INTERVAL;
    return 1;
}

int instrumentFinishMeasureInterval(uint32_t* period_out, uint32_t* interval_out) {
    if (s_instrument_state != STATE_MEASURING || s_measurement_state.mode != MEASUREMENT_INTERVAL)
        return -1;

    uint32_t period, pulse_width;

    if (HWPollIntervalMeasurement(&period, &pulse_width) <= 0)
        return 0;

    *period_out = period;

    if (pulse_width < period)
        *interval_out = pulse_width;
    else {
        // This weird behavior should be investigated, documented and mitigated (in a better way). Any volunteers?
        // To start digging into this, just remove the subtraction and watch what happens when measuring two in-phase signals.
        *interval_out = pulse_width - period;
    }

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

int instrumentSetPwm(size_t index, uint16_t prescaler, uint16_t period, uint16_t pulse_width, uint16_t phase) {
    if (index == 1)
        s_pwm_phase = phase;

    if (HWSetPwm(index, prescaler, period, pulse_width, phase) <= 0)
        return -1;

    if (HWSetPwmPhase(s_pwm_phase) <= 0)
        return -1;

    return 0;
}
