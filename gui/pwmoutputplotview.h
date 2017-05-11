#ifndef PWMOUTPUTPLOTVIEW_H
#define PWMOUTPUTPLOTVIEW_H

#include <QObject>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>

#include "guicommon.h"

class PwmOutputPlotView : public QObject
{
    Q_OBJECT
public:
    explicit PwmOutputPlotView(InstrumentParameterMap& ipm) : ipm(ipm) {}
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

#endif // PWMOUTPUTPLOTVIEW_H
