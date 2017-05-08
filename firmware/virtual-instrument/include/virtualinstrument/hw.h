#ifndef VIRTUALINSTRUMENT_HW_H_
#define VIRTUALINSTRUMENT_HW_H_

#include <stddef.h>
#include <stdint.h>

// Counting measurement
int     HWGetCounterValue(uint32_t* value_out);

void    HWClearPeriodMeasurement(void);
void    HWClearPulseCounter(void);
int     HWInitPeriodMeasurement(size_t num_samples);
// 48.16 frac
int     HWGetPeriodPulseWidth(uint64_t* period_out, uint64_t* pulse_width_out);
void 	HWSetGeneratorPWM(uint16_t prescaler, uint16_t period, uint16_t pulse_time, int phase);
void    HWStartTimeframe(uint32_t duration);        // rename


void    utilDelayMs(uint32_t milliseconds);

// temporary
void HWSetFreqMode(int mode, int edge) __attribute__ ((deprecated ("refactor")));

#endif /* VIRTUALINSTRUMENT_HW_H_ */
