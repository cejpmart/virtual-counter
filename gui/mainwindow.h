#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "measurementcontroller.h"

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
    void measurementShouldStartCounting(double gateTime);
    void measurementShouldStartReciprocal(unsigned int iterations);
    void measurementShouldStartPhase(Edge edge);
    void shouldSetPwmFrequency(double frequency);
    void shouldSetRelativePhase(int phase);
    void shouldOpenInterface(QString path);

private slots:
    void onInstrumentConnected();
    void onInstrumentFirmwareVersionSet(QString text) {}
    void onInstrumentInfoSet(QString text);
    void onMeasurementStarted();
    void onMeasurementFinishedCounting(double frequency, double frequencyError, double period, double periodError);
    void onMeasurementFinishedPhase(double channelAFrequency, double channelAPeriod, double interval, double phase);
    void onMeasurementFinishedReciprocal(double frequency, double frequencyError, double period, double periodError, double duty);
    void onMeasurementTimedOut();
    void onOpenInterfaceTriggered(QAction* action);
    void onPwmFrequencySet(double frequency);
    void onPwmPhaseSet(int phase);

    void on_measureButton_clicked();

    void on_measurementMethodCounting_toggled(bool checked);

    void on_pwmFreqSpinner_valueChanged(double arg1);

    void on_measurementCountingGateSelect_currentIndexChanged(int index);

    void on_continuousMeasurementCheck_toggled(bool checked);

    void on_measurementMethodInterval_toggled(bool checked);

    void on_measurementMethodReciprocal_toggled(bool checked);

    void on_actionQuit_triggered();

    void on_relativePhase_valueChanged(int value);

private:
    double getCountingGateTimeSeconds();
    int getReciprocalIterations();

    void afterMeasurement();

    void fade(QLabel* label);
    void unfade(QLabel* label, const QString& text);

    void setMeasuredValuesFrequencyPeriodDuty(double frequency, double frequencyError, double period, double periodError, double duty);
    void setMeasuredValuesInvalid();
    void setMeasuredValuesFrequencyPeriodIntervalPhase(double channelAFrequency, double channelAPeriod, double interval, double phase);
    void setMeasuredValuesUnknown();
    void statusString(QString text);
    void updateMeasurementFrequencyInfo();

    Ui::MainWindow *ui;

    MeasurementController* measurementController;
    QThread* measurementControllerThread;

    bool settingPwmFrequency = false, pendingPwmFrequency = false;
    double targetPwmFrequency;

    bool settingPwmPhase = false, pendingPwmPhase = false;
    int targetPwmPhase;
};

#endif // MAINWINDOW_H
