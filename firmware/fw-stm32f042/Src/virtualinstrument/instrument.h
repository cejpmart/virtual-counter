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

// Deprecated
enum {
	MODE_COUNTER __attribute__ ((deprecated ("use MEASUREMENT_*"))) = 1,
	MODE_TDELTA __attribute__ ((deprecated ("use MEASUREMENT_*"))) = 3,
};

enum {
	STATE_READY = 1,
	STATE_MEASURING = 2,
	STATE_RESULT_PENDING = 3,
};

// Instrument lifecycle
void instrumentInit(uint32_t cpu_units_per_second);
void instrumentReset(void);
void instrumentProcess(void);

// Measurement Start
int instrumentStartMeasurePulseCount(int gate_time);
int instrumentFinishMeasurePulseCount(unsigned int* count_out);

int instrumentStartMeasurePeriod(unsigned int iterations);
int instrumentFinishMeasurePeriod(uint64_t* period_out, uint64_t* pulse_width_out);

int instrumentStartMeasurePhaseShift();
int instrumentFinishMeasurePhaseShift(unsigned int* period_out, int* interval_out);

int instrumentStartMeasureFreqRatio(unsigned int iterations);
int instrumentFinishMeasureFreqRatio(unsigned int* ratio_out);

// Deprecated, pending clean-up
int instrumentMeasurePhaseAtoB(int* period_out, int* interval_out);
int instrumentGetTdelta(int* tdelta_out);

//void instrumentSetAperture(int ms);

// times are in CPU units
void PulseInfo(uint32_t period, uint32_t pulse_time);

#endif /* VIRTUALINSTRUMENT_INSTRUMENT_H_ */
