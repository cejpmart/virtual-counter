#include "mainwindow.h"
#include "measurementcontroller.h"

static const double F_CPU = 48000000;

MeasurementController::MeasurementController(MainWindow* view) : view(view)
{
}

void MeasurementController::doMeasurementCounting(double gateTime)
{
    if (!session)
        return;

    emit measurementStarted();

    session->writeLine("SENS:FREQ:MODE COUN");
    session->writeLine("SENS:FREQ:GATE:TIME " + QString::number((int)(gateTime * 1000)));
    session->writeLine("MEAS:FREQ?");

    processMeasurement(counting, gateTime);
}

void MeasurementController::doMeasurementPhase()
{
    if (!session)
        return;

    emit measurementStarted();

    session->writeLine("MEAS:PHAS?");

    processPhaseMeasurement();
}

void MeasurementController::doMeasurementReciprocal()
{
    if (!session)
        return;

    emit measurementStarted();

    session->writeLine("SENS:FREQ:MODE REC");
    session->writeLine("MEAS:FREQ?");

    processMeasurement(reciprocal, 0.0);
}

void MeasurementController::openInterface(QString path)
{
    //close();

    printf("Opening %s...\n", path.toLocal8Bit().data());

    session = std::make_unique<SerialSession>();

    //connect(session.get(), SIGNAL (finished()), this, SLOT (interfaceClosed()));
    connect(session.get(), SIGNAL (status(QString)), view, SLOT (statusString(QString)));
    connect(session.get(), SIGNAL (error(QString)), view, SLOT (statusString(QString)));

    if (session->open(path.toLatin1().data())) {
        emit instrumentConnected();
        emit instrumentInfoSet("Connected " + path);
    }
    else {
        session.reset();
    }
}

void MeasurementController::processMeasurement(MeasurementMode mode, double gateTime)
{
    static const double MIN_REASONABLE_FREQUENCY = 0.000001;
    static const double USB_CLOCK_TOLERANCE = 0.0005;       // assuming USB 2.0

    QString result = session->readLine();
    QStringList freqDuty = result.split(",");

    if (freqDuty.size() < 2)
        return;     // FIXME: error

    const double frequency = freqDuty[0].toDouble() * 0.01;
    const double period = (frequency > MIN_REASONABLE_FREQUENCY) ? (1.0 / frequency) : INFINITY;

    if (mode == counting) {
        const double relativeError = USB_CLOCK_TOLERANCE;

        const double frequencyError = ((1 /* off-by-one */) / gateTime) + frequency * relativeError;
        const double periodErrorPoint = qMax(1.0 / (frequency - frequencyError), 0.0);
        const double periodError = periodErrorPoint - period;

        emit measurementFinishedCounting(frequency, frequencyError, period, periodError);
    }
    else {
        const double relativeError = USB_CLOCK_TOLERANCE;

        const double periodError = (1 / F_CPU /* quantization error */) + period * relativeError;
        const double frequencyErrorPoint = qMax(1.0 / (period - periodError), 0.0);
        const double frequencyError = frequencyErrorPoint - frequency;

        const double duty = freqDuty[1].toDouble();

        emit measurementFinishedReciprocal(frequency, frequencyError, period, periodError, duty);
    }
}

void MeasurementController::processPhaseMeasurement()
{
    QString result = session->readLine();
    QStringList freqDuty = result.split(",");

    if (freqDuty.size() < 2)
        return;     // FIXME: error

    const double period = freqDuty[0].toDouble() / F_CPU;
    const double frequency = (period > 1.0 / F_CPU) ? (1.0 / period) : 0.0;

    const double interval = freqDuty[1].toDouble() / F_CPU;
    const double phase = (period > 1.0 / F_CPU) ? (interval / period * 360) : 0.0;

    emit measurementFinishedPhase(frequency, period, -interval, phase);
}

void MeasurementController::setPwmFrequency(double frequency)
{
    if (!session)
        return;

    int period = (int)round(F_CPU / frequency);
    session->writeLine("SOUR:PULS:PER " + QString::number(period));
    double actualFrequency = F_CPU / period;
    emit pwmFrequencySet(actualFrequency);
}

void MeasurementController::setRelativePhase(double phase)
{
    if (!session)
        return;

    session->writeLine("SOUR:PULS:PHAS " + QString::number((int) phase));
}
