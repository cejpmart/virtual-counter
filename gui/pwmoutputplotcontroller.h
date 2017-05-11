#ifndef PWMOUTPUTPLOTCONTROLLER_H
#define PWMOUTPUTPLOTCONTROLLER_H

#include <QObject>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>

#include "guicommon.h"

class PwmOutputPlotController : public QObject
{
    Q_OBJECT
public:
    explicit PwmOutputPlotController(InstrumentParameterMap& ipm) : ipm(ipm) {}
    void init(QtCharts::QChartView* view);

    void redraw(const PwmParameters& pwm1, const PwmParameters& pwm2);
    void resetInstrument();

signals:

public slots:

private:
    InstrumentParameterMap& ipm;

    QtCharts::QChart* chart = nullptr;
    QtCharts::QLineSeries* pwmGraphs[2] = {nullptr, nullptr};
};

#endif // PWMOUTPUTPLOTCONTROLLER_H
