#include "mainwindow.h"
#include "measurementcontroller.h"

#include "../common/protocoldefs.h"

constexpr double USB_CLOCK_TOLERANCE = 0.0005;      // assuming USB 2.0

constexpr uint16_t VERSION = 1008;

constexpr const char* BOARDS[] {
    "Unknown",
    "F042F6",
    "F042K6_Nucleo32",
    "F303_Nucleo64",
    "F411_Nucleo64",
    "F373_Eval",
};

// This is awful, oh, so awful!
static volatile bool measurementAborted = false;

MeasurementController::MeasurementController(MainWindow* view) : view(view)
{
    qRegisterMetaType<Edge>("Edge");
    qRegisterMetaType<InstrumentInfo>("InstrumentInfo");
    qRegisterMetaType<PwmParameters>("PwmParameters");
}

bool MeasurementController::awaitMeasurementResult(uint8_t which, uint8_t const** reply_payload_out, size_t* reply_length_out) {
    measurementAborted = false;

    for (;;) {
        if (measurementAborted) {
            doAbortMeasurement(which);
        }

        uint8_t payload[1];
        payload[0] = which;

        if (!session->sendPacket(CMD_POLL_MEASUREMENT, payload, sizeof(payload))) {
            communicationError();
            return false;
        }

        uint8_t reply_tag;
        const uint8_t* reply_payload;
        size_t reply_length;

        if (!session->awaitPacket(&reply_tag, &reply_payload, &reply_length)) {
            communicationError();
            return false;
        }

        if (reply_tag == INFO_MEASUREMENT_DATA && reply_length >= 1) {
            //reply_payload[0] is rc, always >= 1
            *reply_payload_out = reply_payload + 1;
            *reply_length_out = reply_length - 1;
            return true;
        }
        else if (reply_tag == INFO_RESULT_CODE && reply_length >= 1) {
            int rc = reply_payload[0];
            if (rc == 0)
                continue;
            else {
                qCritical("Measurement error: RESULT_CODE %d\n", reply_payload[0]);
                instrumentStateError();
                break;
            }
        }
    }

    return false;
}

void MeasurementController::closeInterface()
{
    this->session.reset();
}

void MeasurementController::communicationError()
{
    error("Communication error");
}

bool MeasurementController::doAbortMeasurement(uint8_t which)
{
    abort_measurement_request_t request { which };
    int rc;

    if (!sendPacketAndAwaitResultCode(CMD_ABORT_MEASUREMENT, (const uint8_t*) &request, sizeof(request), &rc)) {
        communicationError();
        return false;
    }

    return true;
}

bool MeasurementController::doMeasurement(uint8_t which, const void* request_data, size_t request_length, void* result_data, size_t result_length)
{
    uint8_t payload[100];
    payload[0] = which;
    memcpy(&payload[1], request_data, request_length);
    int rc;

    if (!sendPacketAndAwaitResultCode(CMD_START_MEASUREMENT, payload, 1 + request_length, &rc)) {
        communicationError();
        return false;
    }

    if (rc != RESULT_CODE_OK) {
        instrumentStateError();
        return false;
    }

    emit measurementStarted();

    const uint8_t* reply_payload;
    size_t reply_length;

    if (!awaitMeasurementResult(which, &reply_payload, &reply_length)) {
        return false;
    }

    if (reply_length < result_length) {
        communicationError();
        return false;
    }

    memcpy(result_data, reply_payload, result_length);
    return true;
}

void MeasurementController::doMeasurementCounting(double gateTime)
{
    if (!session)
        return;

    measurement_pulse_count_request_t request;
    measurement_pulse_count_result_t result;

    request.gate_time = (unsigned int)(gateTime * 1000);

    if (!doMeasurement(MEASUREMENT_PULSE_COUNT, &request, sizeof(request), &result, sizeof(result)))
        return;

    auto frequency = result.count / gateTime;

    const double period = (result.count > 0) ? (1.0 / frequency) : INFINITY;

    const double relativeError = USB_CLOCK_TOLERANCE;

    const double frequencyError = ((1 /* off-by-one */) / gateTime) + frequency * relativeError;
    const double periodErrorPoint = qMax(1.0 / (frequency - frequencyError), 0.0);
    const double periodError = periodErrorPoint - period;

    emit measurementFinishedCounting(frequency, frequencyError, period, periodError);
}

void MeasurementController::doMeasurementFreqRatio(unsigned int periods)
{
    if (!session)
        return;

    measurement_freq_ratio_request_t request;
    measurement_freq_ratio_result_t result;

    request.iterations = periods;

    if (!doMeasurement(MEASUREMENT_FREQ_RATIO, &request, sizeof(request), &result, sizeof(result)))
        return;

    emit measurementFinishedFreqRatio(result.ratio / 65536.0, 1.0 / periods);
}

void MeasurementController::doMeasurementPhase(Edge edge)
{
    if (!session)
        return;

    measurement_phase_request_t request;
    measurement_phase_result_t result;

    if (!doMeasurement(MEASUREMENT_INTERVAL, &request, sizeof(request), &result, sizeof(result)))
        return;

    const double period = result.period * (1.0 / f_cpu);
    const double frequency = (result.period > 0) ? (1.0 / period) : 0.0;

    const double interval = result.interval * (1.0 / f_cpu);
    double phase = (result.period > 0) ? (interval / period * 360) : 0.0;

    if (phase > 180)
        phase -= 360;

    emit measurementFinishedPhase(frequency, period, -interval, phase);
}

void MeasurementController::doMeasurementPeriod(unsigned int numPeriods, bool withPhase)
{
    if (!session)
        return;

    measurement_period_request_t request;
    measurement_period_result_t result;

    request.num_periods = numPeriods;

    if (!doMeasurement(withPhase? MEASUREMENT_PWM : MEASUREMENT_PERIOD, &request, sizeof(request), &result, sizeof(result)))
        return;

    const double period = result.period * (1.0 / 65536 / f_cpu);
    const double frequency = (result.period > 0) ? (1.0 / period) : 0.0;

    const double relativeError = USB_CLOCK_TOLERANCE;

    const double periodError = (1 / f_cpu / numPeriods /* quantization error */) + period * relativeError;
    const double frequencyErrorPoint = qMax(1.0 / (period - periodError), 0.0);
    const double frequencyError = frequencyErrorPoint - frequency;

    const double duty = 100.0 * result.pulse_width / result.period;

    emit measurementFinishedPeriod(frequency, frequencyError, period, periodError, duty);
}

void MeasurementController::error(QString&& error) {
    emit status("Error: " + error);
}

bool MeasurementController::getInstrumentInfo(InstrumentInfo& info_out)
{
    Q_ASSERT(session);

    if (!session->sendPacket(CMD_QUERY_INSTRUMENT, nullptr, 0)) {
        communicationError();
        return false;
    }

    uint8_t reply_tag;
    const uint8_t* reply_payload;
    size_t reply_length;

    if (!session->awaitPacket(&reply_tag, &reply_payload, &reply_length)
            || reply_tag != INFO_INSTRUMENT_INFO
            || reply_length < sizeof(instrument_info_t)) {
        communicationError();
        return false;
    }

    instrument_info_t iinfo;
    memcpy(&iinfo, reply_payload, sizeof(iinfo));

    qInfo("board_id=%04X, fw_ver=%d, f_cpu=%d", iinfo.board_id, iinfo.fw_ver, iinfo.f_cpu);

    if (iinfo.fw_ver != VERSION) {
        error("Firmware version mismatch");
        return false;
    }

    info_out.board = BOARDS[iinfo.board_id >> 8];    // FIXME: overflow!!
    info_out.firmware = iinfo.fw_ver;
    info_out.f_cpu = iinfo.f_cpu;

    this->f_cpu = iinfo.f_cpu;

    return true;
}

void MeasurementController::instrumentStateError()
{
    error("Instrument state error");
}

void MeasurementController::openInterface(QString path)
{
    closeInterface();

    printf("Opening %s...\n", path.toLocal8Bit().data());

    auto session = std::make_unique<SerialSession>();

    //connect(session.get(), SIGNAL (finished()), this, SLOT (interfaceClosed()));
    //connect(session.get(), SIGNAL (status(QString)), view, SLOT (statusString(QString)));
    //connect(session.get(), SIGNAL (error(QString)), view, SLOT (statusString(QString)));

    // FIXME EEEE
    connect(this, SIGNAL (status(QString)), view, SLOT (onInstrumentStatusSet(QString)));
    connect(this, SIGNAL (errorSignal(QString)), view, SLOT (onInstrumentStatusSet(QString)));

    try {
        session->open(path.toLatin1().data());
    }
    catch (const std::exception& ex) {
        emit instrumentStatusSet("Failed to open " + path);
        session.reset();

        // TODO: set state
        return;
    }

    this->session = std::move(session);

    InstrumentInfo info;
    info.port = path;

    if (!getInstrumentInfo(info)) {
        this->session.reset();
        return;
    }

    // TODO: load port definitions

    this->session->sendPacket(CMD_RESET_INSTRUMENT, nullptr, 0);

    emit instrumentConnected(info);
    emit instrumentStatusSet("Connected " + path);
}

void MeasurementController::pleaseAbortMeasurement()
{
    measurementAborted = true;
}

bool MeasurementController::sendPacketAndAwaitResultCode(uint8_t tag, const uint8_t* data, size_t length, int* rc_out) {
    if (!session || !session->sendPacket(tag, data, length))
        return false;

    uint8_t reply_tag;
    const uint8_t* reply_payload;
    size_t reply_length;

    if (!session->awaitPacket(&reply_tag, &reply_payload, &reply_length))
        return false;

    if (reply_tag == INFO_RESULT_CODE && reply_length >= 1) {
        *rc_out = (int8_t) reply_payload[0];
        return true;
    }

    return false;
}

void MeasurementController::setPwm(size_t index, PwmParameters params)
{
    if (!session)
        return;

    auto period = f_cpu / params.freq;
    auto pulse_width = period * params.duty;

    while (params.phase < 0)
        params.phase += 360;

    unsigned int prescaler = 1;
    unsigned int prescaled = (unsigned int)round(period);

    while (prescaled >= 65535) {
        prescaler++;
        prescaled = (unsigned int)round(period / prescaler);
    }

    set_pwm_request_t request;
    request.index = index;
    request.prescaler = prescaler - 1;
    request.period = prescaled - 1;
    request.pulse_width = (unsigned int)ceil((float)pulse_width / prescaler);
    request.phase = (unsigned int)round(params.phase * period / prescaler / 360);

    int rc;
    if (!sendPacketAndAwaitResultCode(CMD_SET_PWM, (const uint8_t*) &request, sizeof(request), &rc)) {
        communicationError();
        return;
    }

    // Calculate actual frequency
    params.freq = f_cpu / (prescaler * prescaled);
    params.duty = (float)request.pulse_width / period;
    params.phase = (float)request.phase / request.period * 360;

    if (params.phase > 180)
        params.phase -= 360;

    emit didSetPwm(index, params);
}
