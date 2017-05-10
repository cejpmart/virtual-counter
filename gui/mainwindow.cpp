#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtSerialPort/QSerialPortInfo>

#include <cstdio>

constexpr QChar UNICODE_INFINITY = 0x221E;

constexpr auto MEASUREMENT_PULSE_COUNT_FREQ_RANGE_STRING =  "0 - 24 000 000 Hz";
constexpr auto MEASUREMENT_PERIOD_FREQ_RANGE_STRING =       "0 - 12 000 000 Hz";

// http://stackoverflow.com/a/13094362
static double round_to_digits(double value, int digits)
{
    if (value == 0.0) // otherwise it will return 'nan' due to the log10() of zero
        return 0.0;

    double factor = pow(10.0, digits - ceil(log10(fabs(value))));
    return round(value * factor) / factor;
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->frequencyMeasurementRack->show();
    ui->intervalMeasurementRack->hide();

    setMeasuredValuesUnknown();

    connect(ui->menuOpenInterface, SIGNAL(triggered(QAction*)), this, SLOT(onOpenInterfaceTriggered(QAction*)));

    QMenu* openInterface = ui->menuOpenInterface;

    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();
    for (const QSerialPortInfo& port : ports) {
        QString name = port.portName();

        if (!port.description().isEmpty())
            name += " (" + port.description() + ")";

        QAction* action = new QAction(name, openInterface);
        action->setObjectName(port.systemLocation());
        openInterface->addAction(action);
    }

    openInterface->setDisabled(ports.size() == 0);

    measurementControllerThread = new QThread;
    measurementController = new MeasurementController(this);
    measurementController->moveToThread(measurementControllerThread);
    connect(measurementControllerThread, SIGNAL (finished()), measurementControllerThread, SLOT (deleteLater()));
    connect(measurementControllerThread, SIGNAL (finished()), measurementController, SLOT (deleteLater()));
    measurementControllerThread->start();

    connect(measurementController, SIGNAL (instrumentConnected()), this, SLOT (onInstrumentConnected()));
    connect(measurementController, SIGNAL (instrumentInfoSet(QString)), this, SLOT (onInstrumentInfoSet(QString)));
    connect(measurementController, SIGNAL (instrumentFirmwareVersionSet(QString)), this, SLOT (onInstrumentFirmwareVersionSet(QString)));
    connect(measurementController, SIGNAL (measurementStarted()), this, SLOT (onMeasurementStarted()));
    connect(measurementController, SIGNAL (measurementFinishedCounting(double, double, double, double)),
            this, SLOT (onMeasurementFinishedCounting(double, double, double, double)));
    connect(measurementController, SIGNAL (measurementFinishedReciprocal(double, double, double, double, double)),
            this, SLOT (onMeasurementFinishedReciprocal(double, double, double, double, double)));
    connect(measurementController, SIGNAL (measurementFinishedPhase(double, double, double, double)), this, SLOT (onMeasurementFinishedPhase(double, double, double, double)));
    connect(measurementController, SIGNAL (pwmFrequencySet(double)), this, SLOT (onPwmFrequencySet(double)));
    connect(measurementController, SIGNAL (pwmPhaseSet(int)), this, SLOT (onPwmPhaseSet(int)));
    connect(measurementController, SIGNAL (measurementTimedOut()), this, SLOT (onMeasurementTimedOut()));

    connect(this, SIGNAL (measurementShouldStartCounting(double)), measurementController, SLOT (doMeasurementCounting(double)));
    connect(this, SIGNAL (measurementShouldStartReciprocal(unsigned int)), measurementController, SLOT (doMeasurementReciprocal(unsigned int)));
    connect(this, SIGNAL (measurementShouldStartPhase(Edge)), measurementController, SLOT (doMeasurementPhase(Edge)));
    connect(this, SIGNAL (shouldOpenInterface(QString)), measurementController, SLOT (openInterface(QString)));
    connect(this, SIGNAL (shouldSetPwmFrequency(double)), measurementController, SLOT (setPwmFrequency(double)));
    connect(this, SIGNAL (shouldSetRelativePhase(int)), measurementController, SLOT (setRelativePhase(int)));

    // Auto-connect
    for (const QSerialPortInfo& port : ports) {
        if (port.description() == "Virtual Counter") {
            emit shouldOpenInterface(port.systemLocation());
            break;
        }
    }
}

MainWindow::~MainWindow()
{
    measurementControllerThread->quit();

    delete ui;
}

void MainWindow::afterMeasurement()
{
    statusString((QString) "Ready.");

    if (ui->continuousMeasurementCheck->isChecked())
        on_measureButton_clicked();
}

void MainWindow::fade(QLabel* label)
{
    label->setEnabled(false);
}

double MainWindow::getCountingGateTimeSeconds()
{
    static const double values[] = {0.1, 1.0, 10.0};

    int index = ui->measurementGateTimeSelect->currentIndex();

    if (index >= 0 && (size_t) index < sizeof(values) / sizeof(values[0]))
        return values[index];
    else
        return 0.0;
}

int MainWindow::getReciprocalIterations()
{
    static const int values[] = {1, 10, 100, 1000};

    int index = ui->measurementNumPeriodsSelect->currentIndex();

    if (index >= 0 && (size_t) index < sizeof(values) / sizeof(values[0]))
        return values[index];
    else
        return 1;
}

void MainWindow::onInstrumentConnected()
{
    statusString("Ready.");
    ui->measureButton->setEnabled(true);
    ui->continuousMeasurementCheck->setEnabled(true);
}

void MainWindow::onInstrumentInfoSet(QString text)
{
    ui->instrumentInfoLabel->setText(text);
}

void MainWindow::onMeasurementFinishedCounting(double frequency, double frequencyError, double period, double periodError)
{
    setMeasuredValuesFrequencyPeriodDuty(frequency, frequencyError, period, periodError, 0.0);

    afterMeasurement();
}

void MainWindow::onMeasurementFinishedPhase(double channelAFrequency, double channelAPeriod, double interval, double phase)
{
    setMeasuredValuesFrequencyPeriodIntervalPhase(channelAFrequency, channelAPeriod, interval, phase);

    afterMeasurement();
}

void MainWindow::onMeasurementFinishedReciprocal(double frequency, double frequencyError, double period, double periodError, double duty)
{
    setMeasuredValuesFrequencyPeriodDuty(frequency, frequencyError, period, periodError, duty);

    afterMeasurement();
}

void MainWindow::onMeasurementStarted()
{
    if (!ui->continuousMeasurementCheck->isChecked())
        setMeasuredValuesInvalid();

    statusString((QString) "Measurement in progress...");
}

void MainWindow::onMeasurementTimedOut()
{
    statusString((QString) "Measurement timed out!");

    afterMeasurement();
}

void MainWindow::onOpenInterfaceTriggered(QAction* action)
{
    QString path = action->objectName();
    statusString((QString) "Opening " + path);

    emit shouldOpenInterface(path);
}

void MainWindow::onPwmFrequencySet(double frequency)
{
    ui->pwmFrequencyInfo->setText(QString::number(frequency) + " Hz");

    if (pendingPwmFrequency) {
        pendingPwmFrequency = false;
        emit shouldSetPwmFrequency(targetPwmFrequency);
    }
    else
        settingPwmFrequency = false;
}

void MainWindow::onPwmPhaseSet(int phase)
{
    QString relativePhaseDisplayText;
    relativePhaseDisplayText.sprintf("%+d °", phase);
    ui->relativePhaseDisplay->setText(relativePhaseDisplayText);

    if (pendingPwmPhase) {
        pendingPwmPhase = false;
        emit shouldSetPwmFrequency(targetPwmPhase);
    }
    else
        settingPwmPhase = false;
}

void MainWindow::setMeasuredValuesFrequencyPeriodDuty(double frequency, double frequencyError, double period, double periodError, double duty)
{
    QString frequencyText;
    QString frequencyErrorText;

    if (frequencyError < 1.0) {
        // 24000000.000
        frequencyText.sprintf("%12.3f", frequency);
    }
    else {
        frequencyText.sprintf("%12.0f", frequency);
    }

    frequencyErrorText.sprintf("+/- %g", round_to_digits(frequencyError, 2));

    unfade(ui->measuredFreqValue, frequencyText);
    unfade(ui->measuredFreqErrorValue, frequencyErrorText);

    QString periodText;
    QString periodErrorText;

    if (period == INFINITY) {
        periodText = UNICODE_INFINITY;
    }
    else {
        // FIXME
        periodText.sprintf("%11.9f", period);;
    }

    periodErrorText.sprintf("+/- %g", round_to_digits(periodError, 2));

    unfade(ui->measuredPeriodValue, periodText);
    unfade(ui->measuredPeriodErrorValue, periodErrorText);

    unfade(ui->measuredDutyValue, duty > 0.001 ? QString::number(duty) : QString("N/A"));
}

void MainWindow::setMeasuredValuesInvalid()
{
    fade(ui->measuredFreqValue);
    fade(ui->measuredFreqErrorValue);

    fade(ui->measuredPeriodValue);
    fade(ui->measuredPeriodErrorValue);

    fade(ui->measuredDutyValue);

    fade(ui->channelAFrequency);
    fade(ui->channelAPeriod);
    fade(ui->measuredInterval);
    fade(ui->measuredPhase);
}

void MainWindow::setMeasuredValuesFrequencyPeriodIntervalPhase(double channelAFrequency, double channelAPeriod, double interval, double phase) {
    QString channelAFrequencyText, channelAPeriodText, intervalText, phaseText;
    channelAFrequencyText.sprintf("%12.0f", channelAFrequency);
    channelAPeriodText.sprintf("%11.9f", channelAPeriod);
    intervalText.sprintf("%+11.9f", interval);
    phaseText.sprintf("%+.1f", phase);
    //puts(channelAPeriodText.toLocal8Bit().data());

    unfade(ui->channelAFrequency, channelAFrequencyText);
    unfade(ui->channelAPeriod, channelAPeriodText);
    unfade(ui->measuredInterval, intervalText);
    unfade(ui->measuredPhase, phaseText);
}

void MainWindow::setMeasuredValuesUnknown()
{
    setMeasuredValuesInvalid();

    ui->measuredFreqValue->setText("?");
    ui->measuredFreqErrorValue->setText("?");

    ui->measuredPeriodValue->setText("?");
    ui->measuredPeriodErrorValue->setText("?");

    ui->measuredDutyValue->setText("?");

    ui->channelAFrequency->setText("?");
    ui->channelAPeriod->setText("?");
    ui->measuredInterval->setText("?");
    ui->measuredPhase->setText("?");
}

void MainWindow::statusString(QString text)
{
    ui->instrumentStatusLabel->setText(text);
}

void MainWindow::updateMeasurementFrequencyInfo()
{
    if (ui->measurementMethodCounting->isChecked()) {
        ui->measurementResolutionInfo->setText(QString::number(1.0 / getCountingGateTimeSeconds()) + " Hz");
        ui->measurementRangeInfo->setText(MEASUREMENT_PULSE_COUNT_FREQ_RANGE_STRING);
    }
    else if (ui->measurementMethodPeriod->isChecked()) {
        // FIXME
        ui->measurementResolutionInfo->setText(QString::number(1000000000.0 / 48000000.0) + " ns");
        ui->measurementRangeInfo->setText(MEASUREMENT_PERIOD_FREQ_RANGE_STRING);
    }
}

void MainWindow::unfade(QLabel* label, const QString& text)
{
    label->setEnabled(true);
    label->setText(text);
}

void MainWindow::on_measureButton_clicked()
{
    if (ui->measurementMethodCounting->isChecked()) {
        emit measurementShouldStartCounting(getCountingGateTimeSeconds());
    }
    else if (ui->measurementMethodPeriod->isChecked()) {
        emit measurementShouldStartReciprocal(getReciprocalIterations());
    }
    else if (ui->measurementMethodInterval->isChecked()) {
        //auto edge = ui->intervalEdgeSelect->currentIndex() == 0 ? Edge::rising : Edge::falling;
        emit measurementShouldStartPhase(Edge::rising);
    }
}

void MainWindow::on_measurementMethodCounting_toggled(bool checked)
{
    if (checked) {
        ui->frequencyMeasurementRack->show();
        ui->intervalMeasurementRack->hide();
    }

    ui->measurementGateTimeSelect->setEnabled(checked);
    updateMeasurementFrequencyInfo();
}

void MainWindow::on_pwmFreqSpinner_valueChanged(double arg1)
{
    targetPwmFrequency = arg1;

    if (!settingPwmFrequency) {
        settingPwmFrequency = true;
        emit shouldSetPwmFrequency(arg1);
    }
    else
        pendingPwmFrequency = true;
}

void MainWindow::on_measurementCountingGateSelect_currentIndexChanged(int index)
{
    updateMeasurementFrequencyInfo();
}

void MainWindow::on_continuousMeasurementCheck_toggled(bool checked)
{
    if (checked) {
        on_measureButton_clicked();
    }

    ui->measureButton->setEnabled(!checked);
}

void MainWindow::on_measurementMethodInterval_toggled(bool checked)
{
    if (checked) {
        ui->frequencyMeasurementRack->hide();
        ui->intervalMeasurementRack->show();
    }
}

void MainWindow::on_measurementMethodReciprocal_toggled(bool checked)
{
    if (checked) {
        ui->frequencyMeasurementRack->show();
        ui->intervalMeasurementRack->hide();
    }

    ui->measurementNumPeriodsSelect->setEnabled(checked);
}

void MainWindow::on_actionQuit_triggered()
{
    this->close();
}

void MainWindow::on_relativePhase_valueChanged(int value)
{
    /*QString relativePhaseDisplayText;
    relativePhaseDisplayText.sprintf("%+d °", value);
    ui->relativePhaseDisplay->setText(relativePhaseDisplayText);*/

    targetPwmPhase = value;

    if (!settingPwmPhase) {
        settingPwmPhase = true;
        emit shouldSetRelativePhase(targetPwmPhase);
    }
}
