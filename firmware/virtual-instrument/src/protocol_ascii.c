#include "virtualinstrument/hw.h"
#include "virtualinstrument/protocol.h"
#include "virtualinstrument/instrument.h"

#include "../../../common/protocoldefs.h"

#include <stdio.h>
#include <string.h>

static int s_mode = MEASUREMENT_PULSE_COUNT;
static int s_running = 0;

static int s_gate_time = 1000;

static const int s_continuousInterval = 500;

static const int s_burstCount = 10;
static float s_burstTotal;

static char outbuf[100];

// FIXME: Ouch
extern uint32_t SystemCoreClock;

static void putstr(const char* str) {
	cdcDataOut((const uint8_t*) str, strlen(str));
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

	    uint32_t freq;
		while (instrumentFinishMeasurePulseCount(&freq) <= 0) {
		}

		sprintf(outbuf, "%u Hz\r\n", (unsigned int) freq);

		s_burstTotal += freq;
	}
	else if (s_mode == MEASUREMENT_PERIOD) {
	    if (instrumentStartMeasurePeriod(1, 1) < 0) {
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

		cdcDataOut(data, 1);
		putstr("\r\n");

		switch (*data) {
		case '?':
		case 'h':
			putstr("\r\nCommands:\r\n");
			putstr("[q] Counting\t[w] Reciprocal\t[e] Interval/Phase\r\n");
			putstr("[a] 0.1s Gate\t[s] 1s Gate\t[d] 10s Gate\r\n");
			putstr("[z] Single measurement\t[x] Continous measurement\t[c] Burst (10) measurement\r\n\r\n");
			break;

		case 'q': s_mode = MEASUREMENT_PULSE_COUNT; break;
		case 'w': s_mode = MEASUREMENT_PERIOD; break;
		case 'e': s_mode = MEASUREMENT_INTERVAL; break;

		case 'a': s_gate_time = 100; break;
		case 's': s_gate_time = 1000; break;
		case 'd': s_gate_time = 10000; break;

		case 'z':
			doOneMeasurement();
			break;

		case 'x':
			s_running = 1;
			break;

		case 'c':
			for (int i = 0; i < s_burstCount; i++)
				doOneMeasurement();

			if (s_mode == MEASUREMENT_PULSE_COUNT || s_mode == MEASUREMENT_PERIOD) {
				sprintf(outbuf, "\r\nAverage: %u Hz\r\n", (unsigned int)(s_burstTotal / s_burstCount));
				putstr(outbuf);
			}
			break;
		}
	}

	if (s_running) {
		doOneMeasurement();

		utilDelayMs(s_continuousInterval);
	}
}


