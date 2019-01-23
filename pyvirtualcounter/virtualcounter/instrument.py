import math
import struct
import time

CMD_POLL_MEASUREMENT = ord(b'p')
CMD_START_MEASUREMENT = ord(b's')
CMD_DGEN_OPTIONS = 0x80
CMD_APPLY_DGEN_OPTIONS = 0x81
CMD_RESET_INSTRUMENT = 0xA0
CMD_QUERY_INSTRUMENT = 0xA1
CMD_PROTOCOL_SET_BINARY = 0xF0

DGEN_MODE_ALWAYS_0 = 0
DGEN_MODE_ALWAYS_1 = 1
DGEN_MODE_PWM = 2

MEASUREMENT_PULSE_COUNT = 0x01
MEASUREMENT_PERIOD = 0x02
MEASUREMENT_INTERVAL = 0x04
MEASUREMENT_FREQ_RATIO = 0x05

INFO_RESULT_CODE = 0x10
INFO_MEASUREMENT_DATA = 0x20
INFO_INSTRUMENT_INFO = 0xA1

DEBUG = False
TIMEOUT = 1
VERSION = 1106

class PacketIO:
    def __init__(self, stream):
        self.stream = stream

        self.rxBytes = b''

        # Only needed for Nucleo work-around
        self.totalRxBytes = 0

    def awaitPacket(self):
        start = time.time()

        while time.time() < start + TIMEOUT:
            packet = self.receivePacket()

            if packet:
                return packet
            else:
                #time.sleep(0.01)  # stupid
                break

        raise Exception('awaitPacket timed out')

    def flush(self):
        self.stream.flush()

    def flushInput(self):
        A_BIG_NUMBER = 10000
        self.stream.read(A_BIG_NUMBER)
        while self.stream.in_waiting:
            self.stream.read(self.stream.in_waiting)

    def receivePacket(self):
        header = self.recvall(2, False)

        if header:
            tag, length = struct.unpack('<BB', header)

            rx = self.recvall(2 + length, True)

            # if (enableLogging) {
            #     char buffer[1000];
            #     sprintf(buffer, "tag %02X, length %02X, rx +%d", header[0], header[1], (int)rx);
            #     qInfo("%s", buffer);
            # }

            if rx:
                data = rx[2:]

                # if (enableLogging) {
                #     char dangerousSprintfBuffer[1000];
                #     sprintf(dangerousSprintfBuffer, "tag %02X, length %02X", *tag_out, (unsigned int) *length_out);
                #
                #     if (length) {
                #         strcat(dangerousSprintfBuffer, ", data: [");
                #         for (size_t i = 0; i < length; i++)
                #         sprintf(dangerousSprintfBuffer + strlen(dangerousSprintfBuffer), " %02X", receivedPacketData[2 + i]);
                #         strcat(dangerousSprintfBuffer, "]");
                #     }
                #     qInfo("%s", dangerousSprintfBuffer);
                # }

                return (tag, data)

        return None

    def recvall(self, count, removeFromBuffer):
        if len(self.rxBytes) < count:
            need = count - len(self.rxBytes)
            new_bytes = self.stream.read(need)

            # if (enableLogging) {
            #     char dangerousSprintfBuffer[1000] = "in\t";
            #
            #     for (size_t i = 0; i < got; i++)
            #         sprintf(dangerousSprintfBuffer + strlen(dangerousSprintfBuffer), " %02X", rxBytes[numRxBytes + i]);
            #
            #     qInfo("%s", dangerousSprintfBuffer);
            # }

            if self.totalRxBytes == 0:
                # Throw away spurious 0x00s caused by poor hardware design on Nucleo kits

                while len(new_bytes) and new_bytes[0] == 0:
                    new_bytes = new_bytes[1:]

            self.rxBytes += new_bytes
            self.totalRxBytes += len(new_bytes)

        if len(self.rxBytes) >= count:
            to_return = self.rxBytes[0:count]

            if removeFromBuffer:
                self.rxBytes = self.rxBytes[count:]

            return to_return

        return None

    def sendPacket(self, tag, data):
        if data is None:
            data = b''

        header = struct.pack('<BB', tag, len(data))

        self.stream.write(header + data)
        self.stream.flush()

    def sendPacketAndExpectResultCode(self, tag, data):
        self.sendPacket(tag, data)

        reply_tag, reply_data = self.awaitPacket()

        if reply_tag == INFO_RESULT_CODE and len(reply_data) >= 1:
            return struct.unpack('<B', reply_data)[0]

        raise Exception('Unexpected response packet')


class FrequencyMeasurementFunction:
    def __init__(self, instrument):
        self.instrument = instrument

    def measure_frequency(self, gate_time=1):
        gate_time_ms = int(gate_time * 1000)
        assert gate_time_ms >= 1

        #measurement_pulse_count_request_t request;
        request = struct.pack('<I', gate_time_ms)

        #measurement_pulse_count_result_t result;
        result = self.instrument.doMeasurement(MEASUREMENT_PULSE_COUNT, request)
        count, = struct.unpack('<I', result)

        frequency = count / gate_time

        # const double period = (result.count > 0) ? (1.0 / frequency) : INFINITY;
        #
        # const double frequencyError = ((1 /* off-by-one */) / gateTime) + frequency * getTimebaseRelativeError();
        # const double periodErrorPoint = qMax(1.0 / (frequency - frequencyError), 0.0);
        # const double periodError = periodErrorPoint - period;

        return frequency

    def get_frequency_range(self):
        return (0, self.instrument.get_f_cpu() / 2 - 1)

    def suggest_gate_time(self, frequency, desired_relative_error):
        max_gate_time = 25      # FIXME: this needs to be queried from device
                                # for 16-bit timer: max_gate_time = 2^32 / f_cpu
                                # for 32-bit timer: max_gate_time = 2^64 / f_cpu
                                # However, when using nearly full range, decomposition into <prescaler, cycles> may be non-trivial!
        return min(max(1 / frequency / desired_relative_error, 0.001), max_gate_time)


class FrequencyRatioMeasurementFunction:
    def __init__(self, instrument):
        self.instrument = instrument

    def measure_frequency_ratio(self, num_periods: int):
        # TODO: document num_periods

        #measurement_freq_ratio_request_t request;
        request = struct.pack('<I', num_periods)

        #measurement_freq_ratio_result_t result;
        result = self.instrument.doMeasurement(MEASUREMENT_FREQ_RATIO, request)
        ratio, = struct.unpack('<Q', result)

        return ratio * 2.0**-16


class PeriodMeasurementFunction:
    def __init__(self, instrument):
        self.instrument = instrument

    def measure_period(self, num_periods: int = 1):
        assert num_periods > 0

        #measurement_period_request_t request;
        request = struct.pack('<I', num_periods)

        #measurement_period_result_t result;
        result = self.instrument.doMeasurement(MEASUREMENT_PERIOD, request)
        period_ticks, _ = struct.unpack('<QQ', result)

        period = period_ticks / self.instrument.get_f_cpu() * (2.0 ** -32)

        return period

    def get_frequency_range(self):
        return (0.02, self.instrument.get_f_cpu() / 2 - 1)

    def suggest_num_periods(self, period, desired_relative_error):
        return math.ceil(1 / self.instrument.get_f_cpu() / period / desired_relative_error)

class PhaseMeasurementFunction:
    def __init__(self, instrument):
        self.instrument = instrument

    def measure_period_and_phase(self, num_periods: int = 1):
        assert num_periods > 0

        #measurement_phase_request_t request;
        ch1_falling = 0
        ch2_falling = 0
        request = struct.pack('<BB', ch1_falling, ch2_falling)

        #measurement_phase_result_t result;
        result = self.instrument.doMeasurement(MEASUREMENT_INTERVAL, request)
        period_ticks, interval_ticks = struct.unpack('<II', result)

        period = period_ticks / self.instrument.get_f_cpu()
        phase = 360 * interval_ticks / period_ticks

        return period, phase

class PwmChannel:
    def __init__(self, instrument, chan: int):
        self.instrument = instrument
        self.chan = chan

    def set_frequency(self, frequency_hz: float, phase_deg: float = 0, duty: float = 0.5):
        assert frequency_hz >= 0
        assert duty >= 0 and duty <= 1

        period = self.instrument.get_f_cpu() / frequency_hz
        pulse_width = period * duty

        while phase_deg < 0:
            phase_deg += 360

        prescaler = 1
        prescaled = round(period)

        while prescaled >= 65535:
            prescaler += 1
            prescaled = round(period / prescaler)

        request = struct.pack('<HHHHHH',
                              self.chan,
                              DGEN_MODE_PWM,
                              prescaler - 1,
                              prescaled - 1,
                              math.ceil(pulse_width / prescaler),
                              round(phase_deg * period / prescaler / 360))

        rc = self.instrument.io.sendPacketAndExpectResultCode(CMD_DGEN_OPTIONS, request)
        if DEBUG:
            print('rc=', rc)

        rc = self.instrument.io.sendPacketAndExpectResultCode(CMD_APPLY_DGEN_OPTIONS, None)
        if DEBUG:
            print('rc=', rc)

        # Calculate actual frequency
        # params.freq = f_cpu / (prescaler * prescaled);
        # params.duty = (float)request.pulse_width / request.period;
        # params.phase = (float)request.phase / request.period * 360;
        #
        # if (params.phase > 180)
        # params.phase -= 360;
        #
        # emit didSetPwm(index, params);

        #self.instrument.io.send_packet...

class Instrument:
    def __init__(self, io):
        self.io = io

        # Enter binary protocol
        self.io.sendPacket(CMD_PROTOCOL_SET_BINARY, None)
        self.io.flushInput()

        self.info = self.query_instrument_info()

        self.io.sendPacket(CMD_RESET_INSTRUMENT, None)

    @staticmethod
    def open_serial_port(*args, **kwargs):
        import serial

        return Instrument(PacketIO(serial.Serial(*args, **kwargs)))

    def awaitMeasurementResult(self, which):
        # measurementAborted = false;

        while True:
            # if (measurementAborted) {
            #     doAbortMeasurement(which);
            # }

            payload = struct.pack('<B', which)
            self.io.sendPacket(CMD_POLL_MEASUREMENT, payload);

            reply_tag, reply_data = self.io.awaitPacket()

            if reply_tag == INFO_MEASUREMENT_DATA and len(reply_data) >= 1:
                #reply_data[0] is rc, always >= 1
                return reply_data[1:]
            elif reply_tag == INFO_RESULT_CODE and len(reply_data) >= 1:
                rc, = struct.unpack('<B', reply_data)
                if rc == 0:
                    continue
                else:
                    raise Exception(f"Measurement error: RESULT_CODE {rc}")

        return None

    def doMeasurement(self, which, request):
        payload = struct.pack('<B', which) + request

        RESULT_CODE_OK =1

        rc = self.io.sendPacketAndExpectResultCode(CMD_START_MEASUREMENT, payload)
        assert rc == RESULT_CODE_OK

        result = self.awaitMeasurementResult(which)
        return result

    def get_frequency_measurement_function(self):
        return FrequencyMeasurementFunction(self)

    def get_frequency_ratio_measurement_function(self):
        return FrequencyRatioMeasurementFunction(self)

    def get_f_cpu(self):
        return self.info['f_cpu']

    def get_period_measurement_function(self):
        return PeriodMeasurementFunction(self)

    def get_phase_measurement_function(self):
        return PhaseMeasurementFunction(self)


    def get_pwm_channel(self, chan: int):
        assert chan >= 0 and chan <= 2
        return PwmChannel(self, chan)

    def query_instrument_info(self):
        self.io.sendPacket(CMD_QUERY_INSTRUMENT, None)

        reply_tag, reply_data = self.io.awaitPacket()
        assert reply_tag == INFO_INSTRUMENT_INFO

        board_id, fw_ver, f_cpu, timebase_source = struct.unpack('<HHIB', reply_data)

        if DEBUG:
            print(f"board_id={board_id:04X}, fw_ver={fw_ver}, f_cpu={f_cpu}")

        if fw_ver != VERSION:
            raise Exception("Firmware version not supported")

        return dict(f_cpu=f_cpu)
