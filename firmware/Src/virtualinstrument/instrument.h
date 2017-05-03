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

//enum { kNumChannels = 1 };
//enum { kNumPulseChannels = 1 };

//enum { kChannelDataValid = 0x00000001 };

// Deprecated
enum {
	MODE_COUNTER __attribute__ ((deprecated ("use MEASUREMENT_*"))) = 1,
	MODE_RECIPROCAL __attribute__ ((deprecated ("use MEASUREMENT_*"))) = 2,
	MODE_TDELTA __attribute__ ((deprecated ("use MEASUREMENT_*"))) = 3,
};

enum {
	STATE_READY = 1,
	STATE_MEASURING = 2,
	STATE_RESULT_PENDING = 3,
};
/*
enum {
	MEASUREMENT_PULSE_COUNT = 1,
	MEASUREMENT_PERIOD = 2,
	MEASUREMENT_PHASE = 3,
};
*/
/*typedef struct {
	uint8_t mode;
	uint8_t unused__;
	uint16_t prescaler;
} PulseMeasureConfig;*/

/*typedef struct {
	PulseMeasureConfig conf;
	uint32_t flags;
	uint32_t last_sent;

	uint32_t timespan;
	uint32_t count;
	uint32_t period;
	uint32_t pulse_width;
} PulseMeasureState;*/

extern volatile uint32_t meas_rec_period, meas_rec_pulseWidth;
extern volatile uint32_t meas_rec_valid;

extern volatile uint32_t meas_tdelta_edge1, meas_tdelta_edge2;
extern volatile uint32_t meas_tdelta_valid;

// Instrument lifecycle
void instrumentInit(uint32_t cpu_units_per_second);
void instrumentReset(void);
void instrumentProcess(void);

// Measurement Start
int instrumentStartMeasurePulseCount(int gate_time);
int instrumentFinishMeasurePulseCount(unsigned int* count_out);

int instrumentStartMeasurePeriod(unsigned int iterations);
int instrumentFinishMeasurePeriod(unsigned int* period_out, unsigned int* pulse_width_out);

int instrumentStartMeasurePhaseShift();
int instrumentFinishMeasurePhaseShift(unsigned int* period_out, int* interval_out);

// Deprecated, pending clean-up

//extern volatile uint32_t meas_riseTime, meas_fallTime;
//extern volatile uint32_t meas_tdelta_period1, meas_tdelta_period2;

//void ConfigureChannel(size_t index, const ChannelConfig* conf);
//void ConfigurePulseMeasurement(size_t channel, const PulseMeasureConfig* conf);

//void instrumentSetFreqMode(int freq_mode);
//int instrumentMeasureFrequency(float* freq_out, int* duty_out);
//int instrumentMeasurePeriod(unsigned int* period_out, unsigned int* pulse_out);
int instrumentMeasurePhaseAtoB(int* period_out, int* interval_out);
int instrumentGetTdelta(int* tdelta_out);

//void instrumentSetAperture(int ms);

// times are in CPU units
void PulseInfo(uint32_t period, uint32_t pulse_time);

#endif /* VIRTUALINSTRUMENT_INSTRUMENT_H_ */
