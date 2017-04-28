#include "protocol.h"
#include "instrument.h"

#include "../../common/protocoldefs.h"

#include <string.h>

static const char* s_device_name;

static uint8_t s_rx_packet[16];
static size_t s_rx_have;

struct packet {
    uint8_t tag;
    uint8_t length;
    uint8_t data[];
} __attribute__ ((packed));

void protocolBinaryInit(const char* device_version) {
	s_device_name = device_version;

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
    return cdcDataOut((const uint8_t*) packet, 2 + packet->length);
}

/*static void putbyte(uint8_t value) {
	cdcDataOut(&value, 1);
}*/

static void putstr(const char* str) {
	cdcDataOut((const uint8_t*) str, strlen(str) + 1);
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

        uint8_t reply_buffer[16];
        struct packet* reply_packet = (struct packet*) reply_buffer;

        switch (packet->tag) {
        case CMD_QUERY_VERSION:
            putstr(s_device_name);
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

            case MEASUREMENT_PERIOD: {
                uint32_t iterations = 1;

                if (packet->length >= 5) {
                    memcpy(&iterations, &packet->data[1], 4);
                }

                rc = instrumentStartMeasurePeriod(iterations);
                break;
            }

            case MEASUREMENT_PHASE:
                rc = instrumentStartMeasurePhaseShift();
                break;
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
                uint32_t freq;

                if ((rc = instrumentFinishMeasurePulseCount(&freq)) > 0) {
                    reply_packet->tag = INFO_MEASUREMENT_DATA;
                    reply_packet->length = 5;
                    reply_packet->data[0] = rc;
                    memcpy(&reply_packet->data[1], &freq, 4);
                }
                break;
            }

            case MEASUREMENT_PERIOD: {
                uint32_t period, pulse_width;

                if ((rc = instrumentFinishMeasurePeriod(&period, &pulse_width)) > 0) {
                    reply_packet->tag = INFO_MEASUREMENT_DATA;
                    reply_packet->length = 9;
                    reply_packet->data[0] = rc;
                    memcpy(&reply_packet->data[1], &period, 4);
                    memcpy(&reply_packet->data[5], &pulse_width, 4);
                }
                break;
            }

            case MEASUREMENT_PHASE: {
                uint32_t period;
                int32_t interval;

                if ((rc = instrumentFinishMeasurePhaseShift(&period, &interval)) > 0) {
                    reply_packet->tag = INFO_MEASUREMENT_DATA;
                    reply_packet->length = 9;
                    reply_packet->data[0] = rc;
                    memcpy(&reply_packet->data[1], &period, 4);
                    memcpy(&reply_packet->data[5], &interval, 4);
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
        }

        memmove(s_rx_packet, s_rx_packet + used, s_rx_have);
    }
}
