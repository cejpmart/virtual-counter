/*
 * instrument.h
 *
 *  Created on: Oct 1, 2016
 *      Author: Martin Cejp
 */

#ifndef VIRTUALINSTRUMENT_INSTRUMENT_H_
#define VIRTUALINSTRUMENT_INSTRUMENT_H_

#include <stddef.h>
#include <stdint.h>

enum {
    INSTRUMENT_VERSION = 1007,
};

enum {
    STATE_READY =               1,
    STATE_MEASURING =           2,
    STATE_RESULT_PENDING =      3,
};

// Instrument lifecycle
void instrumentInit(void);
void instrumentReset(void);
void instrumentProcess(void);

// Measurement Start
int instrumentStartMeasurePulseCount(int gate_time);
int instrumentFinishMeasurePulseCount(uint32_t* count_out);

int instrumentStartMeasurePeriod(uint32_t num_periods, int with_pulse_width);
int instrumentFinishMeasurePeriod(uint64_t* period_out, uint64_t* pulse_width_out);

int instrumentStartMeasureInterval();
int instrumentFinishMeasureInterval(uint32_t* period_out, int32_t* interval_out);

int instrumentStartMeasureFreqRatio(uint32_t iterations);
int instrumentFinishMeasureFreqRatio(uint64_t* ratio_out);

// PWM
int instrumentSetPwm(size_t index, uint32_t period, uint32_t pulse_width, uint32_t phase);

#endif /* VIRTUALINSTRUMENT_INSTRUMENT_H_ */
