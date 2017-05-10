#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "guicommon.h"
#include "measurementcontroller.h"
#include "pwmoutputplotcontroller.h"

constexpr size_t NUM_PWM = 2;

namespace Ui {
class MainWindow;
}

class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

signals:
    void shouldOpenInterface(QString path);

    void measurementShouldStartCounting(double gateTime);
    void measurementShouldStartReciprocal(unsigned int iterations);
    void measurementShouldStartPhase(Edge edge);
    void measurementShouldStartFreqRatio(unsigned int periods);

    void shouldSetPwm(size_t index, PwmParameters params);

private slots:
    void onInstrumentConnected();
    void onInstrumentFirmwareVersionSet(QString text);
    void onInstrumentStatusSet(QString text);
    void onMeasurementStarted();
    void onMeasurementFinishedCounting(double frequency, double frequencyError, double period, double periodError);
    void onMeasurementFinishedPhase(double channelAFrequency, double channelAPeriod, double interval, double phase);
    void onMeasurementFinishedReciprocal(double frequency, double frequencyError, double period, double periodError, double duty);
    void onMeasurementFinishedFreqRatio(double ratio);
    void onMeasurementTimedOut();
    void onOpenInterfaceTriggered(QAction* action);
    void onPwmSet(size_t index, PwmParameters params);

    void on_measureButton_clicked();

    void on_measurementMethodCounting_toggled(bool checked);

    void on_measurementCountingGateSelect_currentIndexChanged(int index);

    void on_measurementMethodInterval_toggled(bool checked);

    void on_measurementMethodPeriod_toggled(bool checked);

    void on_actionQuit_triggered();

    void on_pwm1FreqSpinner_valueChanged(double arg1);

    void on_pwm2Phase_valueChanged(int value);

    void on_pwm1DutySlider_valueChanged(int value);

    void on_pwm2DutySlider_valueChanged(int value);

    void on_pwmAEnabled_toggled(bool checked);

    void on_pwmBEnabled_toggled(bool checked);

    void on_pwmBFreqSpinner_valueChanged(double arg1);

    void on_continuousMeasurementToggle_clicked();

    void on_actionAbort_measurement_triggered();

private:
    double getCountingGateTimeSeconds();
    int getReciprocalIterations();

    void afterMeasurement();

    void fade(QLabel* label);
    void unfade(QLabel* label, const QString& text);

    void setContinousMeasurement(bool enabled);
    void setMeasuredValuesFrequencyPeriodDuty(double frequency, double frequencyError, double period, double periodError, double duty);
    void setMeasuredValuesInvalid();
    void setMeasuredValuesFrequencyPeriodIntervalPhase(double channelAFrequency, double channelAPeriod, double interval, double phase);
    void setMeasuredValuesUnknown();
    void statusString(QString text);
    void updateMeasurementFrequencyInfo();

    void updatePwm1();
    void updatePwm2();

    Ui::MainWindow *ui;

    MeasurementController* measurementController;
    QThread* measurementControllerThread;

    bool continuousMeasurement = false;

    PwmOutputPlotController pwmOutputPlotController;

    Parameter<PwmParameters> pwm[NUM_PWM];
    PwmParameters pwmActual[NUM_PWM];
};

#endif // MAINWINDOW_H
