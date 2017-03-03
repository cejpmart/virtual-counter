#include "protocol.h"
#include "instrument.h"

#include <stdio.h>
#include <string.h>

int s_mode = MODE_COUNTER;
int s_running = 0;
static const int s_continuousInterval = 500;

static const int s_burstCount = 10;
static float s_burstTotal;

static char outbuf[100];

// FIXME: Ouch
extern uint32_t SystemCoreClock;

static void putstr(const char* str) {
	cdcDataOut((const uint8_t*) str, strlen(str));
	//cdcDataOut("\r\n");
}

void protocolAsciiInit() {
	//putstr("Virtual Counter\r\n");
	//putstr("press H for Help\r\n\n");
}

static void doOneMeasurement() {
	if (s_mode == MODE_COUNTER || s_mode == MODE_RECIPROCAL) {
		float freq;
		int duty;
		instrumentMeasureFrequency(&freq, &duty);

		if (duty == 0) {
			sprintf(outbuf, "%u Hz\r\n", (unsigned int)(freq));
		}
		else {
			sprintf(outbuf, "%u Hz\t\t%d %%\r\n", (unsigned int)(freq), duty);
		}
		putstr(outbuf);

		s_burstTotal += freq;
	}
	else if (s_mode == MODE_TDELTA) {
		int period, phase;
		instrumentMeasurePhaseAtoB(&period, &phase);

		sprintf(outbuf, "%u Hz, %+d deg phase shift\r\n", (unsigned int)(SystemCoreClock / period), (int)(360 * phase / period));
		putstr(outbuf);
	}
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
		case 'h':
			putstr("\r\nCommands:\r\n");
			putstr("[q] Counting\t[w] Reciprocal\t[e] Interval\r\n");
			putstr("[a] 0.1s Gate\t[s] 1s Gate\t[d] 10s Gate\r\n");
			putstr("[z] Single measurement\t[x] Continous measurement\t[c] Burst (10) measurement\r\n\r\n");
			break;

		case 'q': s_mode = MODE_COUNTER; break;
		case 'w': s_mode = MODE_RECIPROCAL; break;
		case 'e': s_mode = MODE_TDELTA; break;

		case 'a': instrumentSetAperture(100); break;
		case 's': instrumentSetAperture(1000); break;
		case 'd': instrumentSetAperture(10000); break;

		case 'z':
			instrumentSetFreqMode(s_mode);
			doOneMeasurement();
			break;

		case 'x':
			instrumentSetFreqMode(s_mode);
			s_running = 1;
			break;

		case 'c':
			instrumentSetFreqMode(s_mode);

			for (int i = 0; i < s_burstCount; i++)
				doOneMeasurement();

			if (s_mode == MODE_COUNTER || s_mode == MODE_RECIPROCAL) {
				sprintf(outbuf, "\r\nAverage: %u Hz\r\n", (unsigned int)(s_burstTotal / s_burstCount));
				putstr(outbuf);
			}
			break;
		}
	}

	if (s_running) {
		doOneMeasurement();

		HAL_Delay(s_continuousInterval);
	}
}


