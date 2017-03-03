#ifndef MEASUREMENTCONTROLLER_H
#define MEASUREMENTCONTROLLER_H

#include "serialsession.h"

class MainWindow;

class MeasurementController : public QObject
{
    Q_OBJECT
public:
    explicit MeasurementController(MainWindow* view);

signals:
    void instrumentConnected();
    void instrumentInfoSet(QString text);

    void measurementStarted();
    void measurementFinishedCounting(double frequency, double frequencyError, double period, double periodError);
    void measurementFinishedReciprocal(double frequency, double frequencyError, double period, double periodError, double duty);
    void measurementFinishedPhase(double channelAFrequency, double channelAPeriod, double interval, double phase);

    void pwmFrequencySet(double frequency);

public slots:
    void doMeasurementCounting(double gateTime);
    void doMeasurementReciprocal();
    void doMeasurementPhase();

    void setPwmFrequency(double frequency);
    void setRelativePhase(double phase);

    void openInterface(QString path);

private:
    enum MeasurementMode { counting, reciprocal };

    void processMeasurement(MeasurementMode mode, double gateTime);
    void processPhaseMeasurement();

    MainWindow* view;
    std::unique_ptr<SerialSession> session;
};

#endif // MEASUREMENTCONTROLLER_H
