#include "virtualinstrument/protocol.h"

#include "protocol_ascii.h"
#include "protocol_binary.h"

#ifdef ENABLE_SCPI
#include "protocol_scpi.h"
#endif

#include <math.h>
#include <stdio.h>
#include <string.h>

//static const char* s_device_name;
//static uint32_t s_cpu_units_per_second;

//static DeviceId s_device_id;

//static const uint8_t sync[] = {0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA};
//static const uint8_t heartbeat[] = {0xAF};

static uint8_t pendingData[64];
static size_t pendingDataReadPos = 0;
static size_t pendingDataWritePos = 0;

void cdcInitDone(void) {
}

void cdcDataIn(const uint8_t* data, size_t length) {
	for (; length; data++, length--) {
		size_t nextWritePos = (pendingDataWritePos + 1) & (sizeof(pendingData) - 1);

		if (nextWritePos == pendingDataReadPos)
			break;

		pendingData[pendingDataWritePos] = *data;
		pendingDataWritePos = nextWritePos;
	}
}

void protocolInit(const char* device_version, uint32_t cpu_units_per_second) {
	/*s_device_name = device_name;
	s_cpu_units_per_second = cpu_units_per_second;

	strncpy((char*) s_device_id.device_name, device_name, sizeof(s_device_id.device_name));
	s_device_id.cpu_units_per_second = cpu_units_per_second;*/

	protocolAsciiInit();
	protocolBinaryInit(device_version);

#ifdef ENABLE_SCPI
	protocolScpiInit();
#endif
}

enum {
	CONTROL_BINARY,
	CONTROL_ASCII,
#ifdef ENABLE_SCPI
	CONTROL_SCPI,
#endif
};

//#ifdef ENABLE_SCPI
//static int controlMode = CONTROL_SCPI;
//#else
static int controlMode = CONTROL_ASCII;
//#endif

void protocolProcess(void) {
	if (pendingDataReadPos != pendingDataWritePos) {
		if (pendingData[pendingDataReadPos] == 0xf0) {
			controlMode = CONTROL_BINARY;
			pendingDataReadPos = (pendingDataReadPos + 1) & (sizeof(pendingData) - 1);
		}
		if (pendingData[pendingDataReadPos] == 0xf1) {
			controlMode = CONTROL_ASCII;
			pendingDataReadPos = (pendingDataReadPos + 1) & (sizeof(pendingData) - 1);
		}
#ifdef ENABLE_SCPI
		if (pendingData[pendingDataReadPos] == 0xf2) {
			controlMode = CONTROL_SCPI;
			pendingDataReadPos = (pendingDataReadPos + 1) & (sizeof(pendingData) - 1);
		}
#endif
	}

	size_t end = (pendingDataWritePos >= pendingDataReadPos) ? pendingDataWritePos : sizeof(pendingData);

	size_t dataAvailable = (end - pendingDataReadPos) & (sizeof(pendingData) - 1);
	size_t nextReadPos = end & (sizeof(pendingData) - 1);

	switch (controlMode) {
	case CONTROL_BINARY: {
		protocolBinaryHandle(pendingData + pendingDataReadPos, dataAvailable);
		break;
	}

	case CONTROL_ASCII: {
		protocolAsciiHandle(pendingData + pendingDataReadPos, dataAvailable);
		break;
	}

#ifdef ENABLE_SCPI
	case CONTROL_SCPI:
		protocolScpiHandle(pendingData + pendingDataReadPos, dataAvailable);
		break;
#endif
	}

	/*
	case CONTROL_PASSIVE:
		numPendingData = 0;

		auto lastMeasure = HAL_GetTick();

		float freq;
		int duty;
		int tdelta;

		//instrumentSetFreqMode(MODE_COUNTER);
		if (instrumentMeasureFrequency(&freq, &duty) == 0) {
			char buffer[100];
			int len = duty ? sprintf(buffer, "%8d Hz\t%2d %%\r\n", (int)freq, duty) : sprintf(buffer, "%8d Hz\r\n", (int)freq);
			cdcDataOut(buffer, len);
		}
		else if (instrumentGetTdelta(&tdelta) == 0) {
			char buffer[100];
			int tdelta_us_100 = tdelta * (100.0f / 48.0f);
			char x = tdelta_us_100 < 0 ? '-' : '+';
			if (tdelta_us_100 < 0) tdelta_us_100 = -tdelta_us_100;
			int len = sprintf(buffer, "%c%5d.%02d us\r\n", x, tdelta_us_100 / 100, tdelta_us_100 % 100);
			cdcDataOut(buffer, len);
		}

		while (HAL_GetTick() < lastMeasure + 500) {
		}
	}
	*/

	pendingDataReadPos = nextReadPos;
}
