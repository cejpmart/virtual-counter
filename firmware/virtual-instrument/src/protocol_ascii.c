#include "virtualinstrument/hw.h"
#include "virtualinstrument/protocol.h"
#include "virtualinstrument/instrument.h"

#include "../../../common/protocoldefs.h"

#include <stdio.h>
#include <string.h>

static int s_mode = MEASUREMENT_PULSE_COUNT;
static int s_running = 0;

static int s_gate_time = 1000;
static int s_num_periods = 1;

static const unsigned int s_continuousInterval = 500;

static const unsigned int s_burstCount = 10;
static uint64_t s_burstTotal;

static char outbuf[100];

// FIXME: Ouch
extern uint32_t SystemCoreClock;

static void putstr(const char* str) {
	DataOut((const uint8_t*) str, strlen(str));
}

static void printCurrentConfig(void) {
    switch (s_mode) {
    case MEASUREMENT_PULSE_COUNT:
        sprintf(outbuf, "Current Mode: Pulse Count, Gate Time: %d msec\r\n", s_gate_time);
        break;

    case MEASUREMENT_PERIOD:
    case MEASUREMENT_PWM:
        sprintf(outbuf, "Current Mode: Period (Reciprocal), Measured Periods: %d\r\n", s_num_periods);
        break;

    case MEASUREMENT_INTERVAL:
        sprintf(outbuf, "Current Mode: Interval\r\n");
        break;

    case MEASUREMENT_FREQ_RATIO:
        sprintf(outbuf, "Current Mode: Freq. Ratio, Measured Periods: %d\r\n", s_num_periods);
        break;
    }

    putstr(outbuf);
}

static void printError(void) {
    putstr("Unknown error!\r\n");
}

static void doOneMeasurement() {
	if (s_mode == MEASUREMENT_PULSE_COUNT) {
	    if (instrumentStartMeasurePulseCount(s_gate_time) < 0) {
            printError();
            return;
        }

	    uint32_t count;
	    while (instrumentFinishMeasurePulseCount(&count) <= 0) {
	    }

	    unsigned int freq;

        if (s_gate_time < 1000)
            freq = (unsigned int) (count * 1000 / s_gate_time);
        else
            freq = (unsigned int) (count / (s_gate_time / 1000));

		sprintf(outbuf, "%u Hz\r\n", freq);

		s_burstTotal += freq;
	}
	else if (s_mode == MEASUREMENT_PERIOD) {
	    if (instrumentStartMeasurePeriod(s_num_periods, 1) < 0) {
	        printError();
	        return;
	    }

		uint64_t period, pulse_width;
        while (instrumentFinishMeasurePeriod(&period, &pulse_width) <= 0) {
        }

		period >>= 16;
		pulse_width >>= 16;

        unsigned int period_ns = period * 1000 / (SystemCoreClock / 1000000);
        unsigned int pulse_width_ns = pulse_width * 1000 / (SystemCoreClock / 1000000);

        sprintf(outbuf, "%uns period, %uns pulse width\r\n", period_ns, pulse_width_ns);
        putstr(outbuf);

        unsigned int freq = SystemCoreClock / period;

        sprintf(outbuf, "%u Hz\t\t%u %%\r\n", freq, (unsigned int) (100 * pulse_width / period));

        s_burstTotal += freq;
	}
	else if (s_mode == MEASUREMENT_INTERVAL) {
		if (instrumentStartMeasureInterval() < 0) {
            printError();
            return;
        }

		uint32_t period;
		int32_t interval;
		while (instrumentFinishMeasureInterval(&period, &interval) <= 0) {
		}

		sprintf(outbuf, "%u Hz, %+d deg phase shift\r\n", (unsigned int)(SystemCoreClock / period), (int)interval * 360 / (int)period);
	}
	else if (s_mode == MEASUREMENT_FREQ_RATIO) {
        if (instrumentStartMeasureFreqRatio(s_num_periods) < 0) {
            printError();
            return;
        }

        uint64_t freq_ratio;
        while (instrumentFinishMeasureFreqRatio(&freq_ratio) <= 0) {
        }

        sprintf(outbuf, "%u.%06u\r\n", (unsigned int)(freq_ratio / 65536), (unsigned int)((freq_ratio % 65536) * 1000000 / 65536));
    }

	putstr(outbuf);
}

void protocolAsciiInit(void) {
    //putstr("Virtual Counter\r\n");
    //putstr("press H for Help\r\n\n");
}

void protocolAsciiHandle(const uint8_t* data, size_t length) {
	for (; length; data++, length--) {
		if (s_running) {
			s_running = 0;
			continue;
		}

		DataOut(data, 1);
		putstr("\r\n");

		switch (*data) {
		case '?':
		case 'h':
		    // FIXME: print current settings
			putstr("\r\nCommands:\r\n"
			        "[q] Counting\t[w] Reciprocal\t[e] Interval/Phase\t[r] Frequency Ratio\r\n"
			        "[a] 0.1s Gate / 1 period\t[s] 1s Gate / 10 periods\t[d] 10s Gate / 100 periods\t[f] 1000 periods\t[g] 10000 periods\r\n"
			        "[z] Single measurement\t[x] Continuous measurement\t[c] Burst (10) measurement\r\n"
			        "[n] Disable PWM / [m] Enable PWM (1 kHz)\r\n"
			        "\r\n");
			printCurrentConfig();
			break;

		case 'q':
		    s_mode = MEASUREMENT_PULSE_COUNT;
		    printCurrentConfig();
		    putstr("Input pin: " PORT_PULSE_COUNT "\r\n");
		    break;

		case 'w':
		    s_mode = MEASUREMENT_PERIOD;
		    printCurrentConfig();
		    putstr("Input pins: " PORT_PERIOD_1 " " PORT_PERIOD_2 "\r\n");
		    break;

		case 'e':
		    s_mode = MEASUREMENT_INTERVAL;
		    printCurrentConfig();
		    putstr("Input pins: " PORT_INTERVAL_A " " PORT_INTERVAL_B "\r\n");
		    break;

		case 'r':
            s_mode = MEASUREMENT_FREQ_RATIO;
            printCurrentConfig();
            putstr("Input pins: " PORT_FREQ_RATIO_A " " PORT_FREQ_RATIO_B "\r\n");
            break;

		case 'a': s_gate_time = 100; s_num_periods = 1; printCurrentConfig(); break;
		case 's': s_gate_time = 1000; s_num_periods = 10; printCurrentConfig(); break;
		case 'd': s_gate_time = 10000; s_num_periods = 100; printCurrentConfig(); break;
		case 'f': s_num_periods = 1000; printCurrentConfig(); break;
		case 'g': s_num_periods = 10000; printCurrentConfig(); break;

		case 'n':
            instrumentSetPwm(0, SystemCoreClock / 1000000 - 1, 1000 - 1, 0, 0);
            instrumentSetPwm(1, SystemCoreClock / 1000000 - 1, 1000 - 1, 0, 250);
            break;

		case 'm':
		    instrumentSetPwm(0, SystemCoreClock / 1000000 - 1, 1000 - 1, 500, 0);
		    instrumentSetPwm(1, SystemCoreClock / 1000000 - 1, 1000 - 1, 500, 250);
		    putstr("Output pins: " PORT_PWM_A " " PORT_PWM_B "\r\n");
		    break;

		case 'z':
			doOneMeasurement();
			break;

		case 'x':
			s_running = 1;
			break;

		case 'c':
		    s_burstTotal = 0;

			for (int i = 0; i < s_burstCount; i++) {
				doOneMeasurement();
			}

			if (s_mode == MEASUREMENT_PULSE_COUNT || s_mode == MEASUREMENT_PERIOD) {
				sprintf(outbuf, "\r\nAverage: %u Hz\r\n", (unsigned int)(s_burstTotal / s_burstCount));
				putstr(outbuf);
			}
			break;
		}

		putstr("Ready >");
	}

	if (s_running) {
		doOneMeasurement();

		utilDelayMs(s_continuousInterval);
	}
}


