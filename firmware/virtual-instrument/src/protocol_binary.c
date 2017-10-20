#include "virtualinstrument/hw.h"
#include "virtualinstrument/protocol.h"
#include "virtualinstrument/instrument.h"

#include "../../../common/protocoldefs.h"

#include <string.h>

// TODO: compile defs maybe?
static uint16_t s_board_id;
static uint16_t s_instrument_version;
static uint32_t s_f_cpu;
static uint8_t s_timebase_source;

static uint8_t s_rx_packet[32];
static size_t s_rx_have;

static uint8_t reply_buffer[32];

struct packet {
    uint8_t tag;
    uint8_t length;
    uint8_t data[];
} __attribute__ ((packed));

void protocolBinaryInit(uint16_t board_id, uint16_t instrument_version, uint32_t f_cpu, uint8_t timebase_source) {
	s_board_id = board_id;
	s_instrument_version = instrument_version;
	s_f_cpu = f_cpu;
	s_timebase_source = timebase_source;

	s_rx_have = 0;
}

/*static int allocpacket(uint8_t tag, size_t length, struct packet** packet) {
    static uint8_t packet_buffer[8];

    if (2 + length > sizeof(packet_buffer)) {

    }
}*/

static size_t getpacket(const uint8_t* data, size_t length, struct packet** packet) {
    if (length < 2)
        return 0;

    size_t packet_size = 2 + data[1];

    if (length < packet_size)
        return 0;

    *packet = (struct packet*) data;

    return packet_size;
}

static int sendpacket(struct packet* packet) {
    return DataOut((const uint8_t*) packet, 2 + packet->length);
}

void protocolBinaryHandle(const uint8_t* data, size_t length) {
    memcpy(s_rx_packet + s_rx_have, data, length);
    s_rx_have += length;

    while (s_rx_have > 0) {
        struct packet* packet;

        size_t used;

        if (!(used = getpacket(s_rx_packet, s_rx_have, &packet)))
            break;

        s_rx_have -= used;

        struct packet* reply_packet = (struct packet*) reply_buffer;

        switch (packet->tag) {
        case CMD_QUERY_INSTRUMENT: {
            instrument_info_t info = {s_board_id, s_instrument_version, s_f_cpu, s_timebase_source};

            reply_packet->tag = INFO_INSTRUMENT_INFO;
            reply_packet->length = sizeof(info);
            memcpy(reply_packet->data, &info, sizeof(info));
            sendpacket(reply_packet);
            break;
        }

        case CMD_ABORT_MEASUREMENT: {
            abort_measurement_request_t request;

            if (packet->length != sizeof(request))
                break;

            memcpy(&request, &packet->data[0], sizeof(request));

            int rc = instrumentAbortMeasurement(request.which);

            reply_packet->tag = INFO_RESULT_CODE;
            reply_packet->length = 1;
            reply_packet->data[0] = (uint8_t) (rc & 0xff);
            sendpacket(reply_packet);
            break;
        }

        case CMD_RESET_INSTRUMENT:
            instrumentReset();
            break;

        case CMD_START_MEASUREMENT: {
            if (packet->length < 1)
                break;

            uint8_t which = packet->data[0];

            int rc = -1;

            switch (which) {
            case MEASUREMENT_PULSE_COUNT: {
                if (packet->length != 5)
                    break;

                uint32_t gate_time_ms;
                memcpy(&gate_time_ms, &packet->data[1], 4);

                rc = instrumentStartMeasurePulseCount(gate_time_ms);
                break;
            }

            case MEASUREMENT_PERIOD:
            case MEASUREMENT_PWM: {
                uint32_t num_periods = 1;

                if (packet->length >= 5) {
                    memcpy(&num_periods, &packet->data[1], 4);
                }

                rc = instrumentStartMeasurePeriod(num_periods, (which == MEASUREMENT_PWM));
                break;
            }

            case MEASUREMENT_INTERVAL: {
            	measurement_phase_request_t request;

                if (packet->length != 1 + sizeof(request))
                    break;

                memcpy(&request, &packet->data[1], sizeof(request));

                rc = instrumentStartMeasureInterval(request.ch1_falling, request.ch2_falling);
                break;
            }

            case MEASUREMENT_FREQ_RATIO: {
                if (packet->length != 5)
                    break;

                uint32_t periods;
                memcpy(&periods, &packet->data[1], 4);

                rc = instrumentStartMeasureFreqRatio(periods);
                break;
            }
            }

            reply_packet->tag = INFO_RESULT_CODE;
            reply_packet->length = 1;
            reply_packet->data[0] = (uint8_t) (rc & 0xff);
            sendpacket(reply_packet);
            break;
        }

        case CMD_POLL_MEASUREMENT:
            if (packet->length != 1)
                break;

            uint8_t which = packet->data[0];

            int rc = -1;

            switch (which) {
            case MEASUREMENT_PULSE_COUNT: {
                uint32_t count;

                if ((rc = instrumentFinishMeasurePulseCount(&count)) > 0) {
                    reply_packet->tag = INFO_MEASUREMENT_DATA;
                    reply_packet->length = 5;
                    reply_packet->data[0] = rc;
                    memcpy(&reply_packet->data[1], &count, 4);
                }
                break;
            }

            case MEASUREMENT_PERIOD:
            case MEASUREMENT_PWM: {
                uint64_t period, pulse_width;

                if ((rc = instrumentFinishMeasurePeriod(&period, &pulse_width)) > 0) {
                    reply_packet->tag = INFO_MEASUREMENT_DATA;
                    reply_packet->length = 1 + 8 + 8;
                    reply_packet->data[0] = rc;
                    memcpy(&reply_packet->data[1], &period, 8);
                    memcpy(&reply_packet->data[9], &pulse_width, 8);
                }
                break;
            }

            case MEASUREMENT_INTERVAL: {
                uint32_t period;
                int32_t interval;

                if ((rc = instrumentFinishMeasureInterval(&period, &interval)) > 0) {
                    reply_packet->tag = INFO_MEASUREMENT_DATA;
                    reply_packet->length = 9;
                    reply_packet->data[0] = rc;
                    memcpy(&reply_packet->data[1], &period, 4);
                    memcpy(&reply_packet->data[5], &interval, 4);
                }
                break;
            }

            case MEASUREMENT_FREQ_RATIO: {
                uint64_t ratio;

                if ((rc = instrumentFinishMeasureFreqRatio(&ratio)) > 0) {
                    reply_packet->tag = INFO_MEASUREMENT_DATA;
                    reply_packet->length = 1 + 8;
                    reply_packet->data[0] = rc;
                    memcpy(&reply_packet->data[1], &ratio, 8);
                }
                break;
            }
            }

            if (rc <= 0) {
                reply_packet->tag = INFO_RESULT_CODE;
                reply_packet->length = 1;
                reply_packet->data[0] = (uint8_t) (rc & 0xff);
            }

            sendpacket(reply_packet);
            break;

        case CMD_SET_PWM: {
            set_pwm_request_t request;

            if (packet->length != sizeof(request))
                break;

            memcpy(&request, &packet->data[0], sizeof(request));

            int rc = instrumentSetPwm(request.index, request.prescaler, request.period, request.pulse_width, request.phase);

            reply_packet->tag = INFO_RESULT_CODE;
            reply_packet->length = 1;
            reply_packet->data[0] = (uint8_t) (rc & 0xff);
            sendpacket(reply_packet);
            break;
        }

        case CMD_PROTOCOL_SET_BINARY:
            break;

        default: {
            reply_packet->tag = INFO_RESULT_CODE;
            reply_packet->length = 1;
            reply_packet->data[0] = RESULT_CODE_PROTOCOL_ERROR;
            sendpacket(reply_packet);
            break;
        }
        }

        memmove(s_rx_packet, s_rx_packet + used, s_rx_have);
    }
}
