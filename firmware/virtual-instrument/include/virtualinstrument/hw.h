#ifndef VIRTUALINSTRUMENT_HW_H_
#define VIRTUALINSTRUMENT_HW_H_

#include <stddef.h>
#include <stdint.h>

// Pulse Count measurement
int     HWStartPulseCountMeasurement(uint32_t gate_time_ms);
int     HWPollPulseCountMeasurement(uint32_t* value_out);

// Period measurement
int     HWStartPeriodMeasurement(size_t num_periods);
// 48.16 frac
int     HWPollPeriodMeasurement(uint64_t* period_out, uint64_t* pulse_width_out);

// Interval measurement
int     HWStartIntervalMeasurement(void);
int     HWPollIntervalMeasurement(uint32_t* period_out, uint32_t* pulse_width_out);

// Frequency ratio measurement
int     HWStartFreqRatioMeasurement(size_t num_periods);
// 48.16 frac
int     HWPollFreqRatioMeasurement(uint64_t* ratio_out);

// PWM
int     HWSetPwm(size_t index, uint16_t prescaler, uint16_t period, uint16_t pulse_time, int phase);
int     HWSetPwmPhase(uint32_t phase);

void    utilDelayMs(uint32_t milliseconds);


#endif /* VIRTUALINSTRUMENT_HW_H_ */
