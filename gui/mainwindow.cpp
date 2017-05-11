#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtSerialPort/QSerialPortInfo>

#include <cstdio>

constexpr QChar UNICODE_INFINITY = 0x221E;

constexpr auto MEASUREMENT_PULSE_COUNT_FREQ_RANGE_STRING =  "0 - 24 000 000 Hz";
constexpr auto MEASUREMENT_PERIOD_FREQ_RANGE_STRING =       "0 - 1 000 000 Hz";

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
    ui(new Ui::MainWindow),
    pwmOutputPlotController(ipm)
{
    qRegisterMetaType<size_t>("size_t");

    ui->setupUi(this);
    pwmOutputPlotController.init(ui->pwmOutputPlot);
    pwmOutputPlotController.redraw(pwmActual[0], pwmActual[1]);

    ui->instrumentDeviceLabel->setText("Not connected");
    ui->instrumentFirmwareLabel->setText("--");
    ui->instrumentStatusLabel->setText("--");
    onMeasurementMethodChanged();
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

    connect(measurementController, SIGNAL (instrumentConnected(InstrumentInfo)), this, SLOT (onInstrumentConnected(InstrumentInfo)));
    connect(measurementController, SIGNAL (measurementStarted()), this, SLOT (onMeasurementStarted()));
    connect(measurementController, SIGNAL (measurementFinishedCounting(double, double, double, double)),
            this, SLOT (onMeasurementFinishedCounting(double, double, double, double)));
    connect(measurementController, SIGNAL (measurementFinishedReciprocal(double, double, double, double, double)),
            this, SLOT (onMeasurementFinishedReciprocal(double, double, double, double, double)));
    connect(measurementController, SIGNAL (measurementFinishedPhase(double, double, double, double)),
            this, SLOT (onMeasurementFinishedPhase(double, double, double, double)));
    connect(measurementController, SIGNAL (didSetPwm(size_t, PwmParameters)), this, SLOT (onPwmSet(size_t, PwmParameters)));
    connect(measurementController, SIGNAL (measurementFinishedFreqRatio(double, double)),
            this, SLOT (onMeasurementFinishedFreqRatio(double, double)));
    connect(measurementController, SIGNAL (measurementTimedOut()), this, SLOT (onMeasurementTimedOut()));

    connect(this, SIGNAL (measurementShouldStartCounting(double)), measurementController, SLOT (doMeasurementCounting(double)));
    connect(this, SIGNAL (measurementShouldStartReciprocal(unsigned int)), measurementController, SLOT (doMeasurementReciprocal(unsigned int)));
    connect(this, SIGNAL (measurementShouldStartPhase(Edge)), measurementController, SLOT (doMeasurementPhase(Edge)));
    connect(this, SIGNAL (measurementShouldStartFreqRatio(unsigned int)), measurementController, SLOT (doMeasurementFreqRatio(unsigned int)));
    connect(this, SIGNAL (shouldOpenInterface(QString)), measurementController, SLOT (openInterface(QString)));
    connect(this, SIGNAL (shouldSetPwm(size_t, PwmParameters)), measurementController, SLOT (setPwm(size_t, PwmParameters)));

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

    if (continuousMeasurement)
        on_measureButton_clicked();
    else
        ui->measureButton->setEnabled(true);
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

void MainWindow::loadIpm(QString boardName)
{
    ipm.clear();

    auto dir = QCoreApplication::applicationDirPath();
    auto fileName = dir + "/" + boardName + ".txt";

    QFile inputFile(fileName);
    QVector<QString> tokens;

    if (inputFile.open(QIODevice::ReadOnly)) {
        QTextStream in(&inputFile);

        while (!in.atEnd()) {
            QString line = in.readLine();

            if (line.size() == 0 || line[0] == '#')
                continue;

            auto tokens = line.split('=');

            if (tokens.size() >= 2) {
                ipm.insert(tokens[0], tokens[1]);
            }
        }

        inputFile.close();
    }
    else
        qWarning("Failed to open '%s'", qPrintable(fileName));
}

void MainWindow::onInstrumentConnected(InstrumentInfo info)
{
    loadIpm(info.board);
    pwmOutputPlotController.resetInstrument();

    pwm[0].setpoint = {true, (float) ui->pwm1FreqSpinner->value(), ui->pwm1DutySlider->value() / 100.0f, 0};
    pwm[1].setpoint = {true, (float) ui->pwmBFreqSpinner->value(), ui->pwm2DutySlider->value() / 100.0f, (float) ui->pwm2Phase->value()};

    for (size_t i = 0; i < NUM_PWM; i++) {
        if (pwm[i].startSetting())
            emit shouldSetPwm(i, pwm[i].setpoint);
    }

    ui->instrumentDeviceLabel->setText("Connected (" + info.port + ")");
    ui->instrumentFirmwareLabel->setText(info.board + "," + QString::number(info.firmware));

    statusString("Ready.");
    ui->measureButton->setEnabled(true);
    ui->continuousMeasurementToggle->setEnabled(true);
}

void MainWindow::onInstrumentStatusSet(QString text)
{
    ui->instrumentStatusLabel->setText(text);
}

void MainWindow::onMeasurementFinishedCounting(double frequency, double frequencyError, double period, double periodError)
{
    setMeasuredValuesFrequencyPeriodDuty(frequency, frequencyError, period, periodError, 0.0);

    afterMeasurement();
}

void MainWindow::onMeasurementFinishedFreqRatio(double freqRatio, double freqRatioError)
{
    setMeasuredValuesFreqRatio(freqRatio, freqRatioError);

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

void MainWindow::onMeasurementMethodChanged()
{
    ui->measurementGateTimeSelect->setEnabled(ui->measurementMethodCounting->isChecked());
    ui->measurementNumPeriodsSelect->setEnabled(ui->measurementMethodPeriod->isChecked() || ui->measurementMethodFreqRatio->isChecked());

    // Racks
    if (ui->measurementMethodCounting->isChecked() || ui->measurementMethodPeriod->isChecked()) {
        ui->frequencyMeasurementRack->show();
        ui->intervalMeasurementRack->hide();
        ui->freqRatioMeasurementRack->hide();
    }
    else if (ui->measurementMethodInterval->isChecked()) {
        ui->frequencyMeasurementRack->hide();
        ui->intervalMeasurementRack->show();
        ui->freqRatioMeasurementRack->hide();
    }
    else if (ui->measurementMethodFreqRatio->isChecked()) {
        ui->frequencyMeasurementRack->hide();
        ui->intervalMeasurementRack->hide();
        ui->freqRatioMeasurementRack->show();
    }

    updateMeasurementFrequencyInfo();
}

void MainWindow::onMeasurementStarted()
{
    if (!continuousMeasurement)
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

void MainWindow::onPwmSet(size_t index, PwmParameters params)
{
    if (index == 0) {
        ui->pwm1FreqLabel->setText(QString::asprintf("%.2f Hz", params.freq));
        ui->pwmADutyLabel->setText(QString::asprintf("%d %%", (int)(100 * params.duty)));
    }
    else if (index == 1) {
        ui->pwm2FreqLabel->setText(QString::asprintf("%.2f Hz", params.freq));
        ui->pwm2PhaseLabel->setText(QString::asprintf("%+d °", (int) params.phase));
        ui->pwmBDutyLabel->setText(QString::asprintf("%d %%", (int)(100 * params.duty)));
    }

    if (pwm[index].continuePending())
        emit shouldSetPwm(index, pwm[index].setpoint);

    pwmActual[index] = params;
    pwmOutputPlotController.redraw(pwmActual[0], pwmActual[1]);
}

void MainWindow::setContinousMeasurement(bool enabled)
{
    continuousMeasurement = enabled;

    if (continuousMeasurement) {
        on_measureButton_clicked();
    }
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
        // FIXME: number formating, number of decimal places
        periodText.sprintf("%11.9f", period);;
    }

    periodErrorText.sprintf("+/- %g", round_to_digits(periodError, 2));

    unfade(ui->measuredPeriodValue, periodText);
    unfade(ui->measuredPeriodErrorValue, periodErrorText);

    unfade(ui->measuredDutyValue, duty > 0.001 ? QString::number(duty) : QString("N/A"));
}

void MainWindow::setMeasuredValuesFreqRatio(double freqRatio, double freqRatioError)
{
    unfade(ui->measuredFreqRatioValue, QString::asprintf("%.2f", freqRatio));
    unfade(ui->measuredFreqRatioError, QString::asprintf("+/- %.2f", freqRatioError));
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

    fade(ui->measuredFreqRatioValue);
    fade(ui->measuredFreqRatioError);
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

    ui->measuredFreqRatioValue->setText("?");
    ui->measuredFreqRatioError->setText("?");
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
        // FIXME: number formatting, correctnes
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
    ui->measureButton->setEnabled(false);

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
    else if (ui->measurementMethodFreqRatio->isChecked()) {
        emit measurementShouldStartFreqRatio(getReciprocalIterations());
    }
}

void MainWindow::on_measurementGateTimeSelect_currentIndexChanged(int index)
{
    updateMeasurementFrequencyInfo();
}

void MainWindow::on_measurementMethodCounting_toggled(bool checked)
{
    onMeasurementMethodChanged();
}

void MainWindow::on_measurementMethodFreqRatio_toggled(bool checked)
{
    onMeasurementMethodChanged();
}

void MainWindow::on_measurementMethodInterval_toggled(bool checked)
{
    onMeasurementMethodChanged();
}

void MainWindow::on_measurementMethodPeriod_toggled(bool checked)
{
    onMeasurementMethodChanged();
}

void MainWindow::on_actionQuit_triggered()
{
    this->close();
}

void MainWindow::on_continuousMeasurementToggle_clicked()
{
    setContinousMeasurement(!continuousMeasurement);

    if (continuousMeasurement)
        ui->continuousMeasurementToggle->setText("Stop");
    else
        ui->continuousMeasurementToggle->setText("Run");
}

void MainWindow::on_pwmAEnabled_toggled(bool checked)
{
    pwm[0].setpoint.enabled = checked;

    if (pwm[0].startSetting())
        emit shouldSetPwm(0, pwm[0].setpoint);
}

void MainWindow::on_pwmBEnabled_toggled(bool checked)
{
    pwm[1].setpoint.enabled = checked;

    if (pwm[1].startSetting())
        emit shouldSetPwm(1, pwm[1].setpoint);
}

void MainWindow::on_pwmBFreqSpinner_valueChanged(double arg1)
{
    pwm[1].setpoint.freq = arg1;

    if (pwm[1].startSetting())
        emit shouldSetPwm(1, pwm[1].setpoint);
}

void MainWindow::on_pwm1DutySlider_valueChanged(int value)
{
    pwm[0].setpoint.duty = value / 100.0f;

    if (pwm[0].startSetting())
        emit shouldSetPwm(0, pwm[0].setpoint);
}

void MainWindow::on_pwm1FreqSpinner_valueChanged(double arg1)
{
    pwm[0].setpoint.freq = arg1;

    if (pwm[0].startSetting())
        emit shouldSetPwm(0, pwm[0].setpoint);
}

void MainWindow::on_pwm2DutySlider_valueChanged(int value)
{
    pwm[1].setpoint.duty = value / 100.0f;

    if (pwm[1].startSetting())
        emit shouldSetPwm(1, pwm[1].setpoint);
}

void MainWindow::on_pwm2Phase_valueChanged(int value)
{
    pwm[1].setpoint.phase = value;

    if (pwm[1].startSetting())
        emit shouldSetPwm(1, pwm[1].setpoint);
}

void MainWindow::on_actionAbort_measurement_triggered()
{
    //measurementController->abortMeasurement();

    afterMeasurement();
}
