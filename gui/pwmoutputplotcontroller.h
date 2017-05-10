#ifndef PWMOUTPUTPLOTCONTROLLER_H
#define PWMOUTPUTPLOTCONTROLLER_H

#include <QObject>

#include "qcustomplot.h"

#include "guicommon.h"

class PwmOutputPlotController : public QObject
{
    Q_OBJECT
public:
    explicit PwmOutputPlotController();

    void redraw(const PwmParameters& pwm1, const PwmParameters& pwm2);
    void setPlot(QCustomPlot* plot);

signals:

public slots:

private:
    QCustomPlot* plot = nullptr;
    QCPGraph* pwmGraphs[2];// = {nullptr, nullptr};
};

#endif // PWMOUTPUTPLOTCONTROLLER_H
