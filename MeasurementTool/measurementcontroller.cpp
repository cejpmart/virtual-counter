#include "mainwindow.h"
#include "measurementcontroller.h"

#include "../Common/protocoldefs.h"

static const double F_CPU = 48000000;
static const double MIN_REASONABLE_FREQUENCY = 0.000001;
static const double MIN_REASONABLE_PERIOD = 1.0 / F_CPU;
static const double USB_CLOCK_TOLERANCE = 0.0005;       // assuming USB 2.0

constexpr const char VERSION[] = "1002";

MeasurementController::MeasurementController(MainWindow* view) : view(view)
{
    qRegisterMetaType<Edge>("Edge");
}

bool MeasurementController::awaitMeasurementResult(uint8_t which, uint8_t const** reply_payload_out, size_t* reply_length_out) {
    for (;;) {
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

bool MeasurementController::checkFirmwareVersion()
{
    Q_ASSERT(session);

    QString versionInfo;

    if (!session->sendPacket(CMD_QUERY_VERSION, nullptr, 0)
            || !session->readString(versionInfo)) {
        communicationError();
        return false;
    }

    QStringList tokens = versionInfo.split(",");

    if (tokens.size() >= 3)
        qCritical("%d `%s` `%s`", tokens.size(), qPrintable(tokens[2]), VERSION);

    if (tokens.size() < 3 || tokens[2] != VERSION) {
        error("Firmware version mismatch");
        return false;
    }

    return true;
}

void MeasurementController::closeInterface()
{
    this->session.reset();
}

void MeasurementController::communicationError()
{
    error("Communication error");
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

    auto frequency = result.frequency;

    const double period = (frequency > MIN_REASONABLE_FREQUENCY) ? (1.0 / frequency) : INFINITY;

    const double relativeError = USB_CLOCK_TOLERANCE;

    const double frequencyError = ((1 /* off-by-one */) / gateTime) + frequency * relativeError;
    const double periodErrorPoint = qMax(1.0 / (frequency - frequencyError), 0.0);
    const double periodError = periodErrorPoint - period;

    emit measurementFinishedCounting(frequency, frequencyError, period, periodError);
}

void MeasurementController::doMeasurementPhase(Edge edge)
{
    if (!session)
        return;

    measurement_phase_request_t request;
    measurement_phase_result_t result;

    if (!doMeasurement(MEASUREMENT_PHASE, &request, sizeof(request), &result, sizeof(result)))
        return;

    const double period = result.period * (1.0 / F_CPU);
    const double frequency = (period > MIN_REASONABLE_PERIOD) ? (1.0 / period) : 0.0;

    const double interval = result.interval * (1.0 / F_CPU);
    const double phase = (period > MIN_REASONABLE_PERIOD) ? (interval / period * 360) : 0.0;

    emit measurementFinishedPhase(frequency, period, -interval, phase);
}

void MeasurementController::doMeasurementReciprocal()
{
    if (!session)
        return;

    measurement_period_request_t request;
    measurement_period_result_t result;

    request.iterations = 1;

    if (!doMeasurement(MEASUREMENT_PERIOD, &request, sizeof(request), &result, sizeof(result)))
        return;

    const double period = result.period * (1.0 / F_CPU);
    const double frequency = (period > MIN_REASONABLE_PERIOD) ? (1.0 / period) : 0.0;

    const double relativeError = USB_CLOCK_TOLERANCE;

    const double periodError = (1 / F_CPU /* quantization error */) + period * relativeError;
    const double frequencyErrorPoint = qMax(1.0 / (period - periodError), 0.0);
    const double frequencyError = frequencyErrorPoint - frequency;

    const double duty = 100.0 * result.pulse_width / result.period;

    emit measurementFinishedReciprocal(frequency, frequencyError, period, periodError, duty);
}

void MeasurementController::error(QString&& error) {
    emit status("Error: " + error);
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
    connect(this, SIGNAL (status(QString)), view, SLOT (onInstrumentInfoSet(QString)));
    connect(this, SIGNAL (errorSignal(QString)), view, SLOT (onInstrumentInfoSet(QString)));

    try {
        session->open(path.toLatin1().data());
    }
    catch (const std::exception& ex) {
        emit instrumentInfoSet("Failed to open " + path);
        session.reset();
    }

    this->session = std::move(session);

    if (!checkFirmwareVersion()) {
        this->session.reset();
        return;
    }

    emit instrumentConnected();
    emit instrumentInfoSet("Connected " + path);
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

void MeasurementController::setPwmFrequency(double frequency)
{
# if 0
    if (!session)
        return;

    int period = (int)round(F_CPU / frequency);
    session->writeLine("SOUR:PULS:PER " + QString::number(period));
    double actualFrequency = F_CPU / period;
    emit pwmFrequencySet(actualFrequency);
#endif
}

void MeasurementController::setRelativePhase(double phase)
{
#if 0
    if (!session)
        return;

    session->writeLine("SOUR:PULS:PHAS " + QString::number((int) phase));
#endif
}
