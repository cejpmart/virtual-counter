/*
 * protocol.h
 *
 *  Created on: Oct 10, 2016
 *      Author: Martin Cejp
 */

#ifndef VIRTUALINSTRUMENT_PROTOCOL_H_
#define VIRTUALINSTRUMENT_PROTOCOL_H_

#include <stddef.h>
#include <stdint.h>

/* host -> device */
enum {
	cmdQueryDeviceId = '?',
	cmdConfigureChannel0 = '0',
	cmdConfigurePulseMeasurement = 'p',
	cmdReset = 'R',
};

enum {
	kChannelModeOff = 0,
	kChannelModeDigital,
};

enum {
	kSampleRate1kSps,
};

typedef enum {
	kPulseMeasureOff = 0,
	kPulseMeasureAll = 1,
} PulseMeasurementMode;

/*
enum {
	BITRATE_10kbit =	0,
	BITRATE_20kbit =	1,
	BITRATE_50kbit =	2,
	BITRATE_100kbit =	3,
	BITRATE_125kbit =	4,
	BITRATE_250kbit =	5,
	BITRATE_500kbit =	6,
	BITRATE_800kbit =	7,
	BITRATE_1Mbit =		8,
};*/

/* device -> host */
enum {
	msgDeviceId = 0x1000,
	msgDataPulseLength = 0x1234,
};

typedef struct {
	uint8_t channel;
	uint8_t mode;
} CmdConfigPulseMeasurement;

typedef struct {
	uint16_t id;
	uint16_t length;
} PacketHeader;

typedef struct {
	PacketHeader hdr;
	uint8_t device_name[28];
	uint32_t cpu_units_per_second;
} DeviceId;

typedef struct {
	PacketHeader hdr;
	uint32_t timespan;
	uint32_t count;
	uint32_t period;
	uint32_t pulse_width;
} DataPulse;

void cdcInitDone(void);
void cdcDataIn(const uint8_t* data, size_t length);
int cdcDataOut(const uint8_t* data, size_t length);

// instrument.c -> protocol.c

void protocolInit(uint16_t board_id, uint16_t instrument_version, uint32_t f_cpu);
void protocolProcess(void);

//void protocolSendHeartbeat(void);	/* TODO: self-handle */
//void protocolMessageReceivedStd(uint16_t sid, const uint8_t* data, size_t length);

void protocolSendPulseInfoTEST(DataPulse* data);

// protocol.c -> instrument.c


#endif /* VIRTUALINSTRUMENT_PROTOCOL_H_ */
