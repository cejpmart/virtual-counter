#include "pwmoutputplotcontroller.h"

PwmOutputPlotController::PwmOutputPlotController()
{
}

void PwmOutputPlotController::redraw(const PwmParameters& pwm1, const PwmParameters& pwm2)
{
    qInfo("redraw");

    auto graphData = graph->data();

    QVector<QCPGraphData> newData;
    double pwm1period = (1.0 / pwm1.freq) * 1000000;
    newData.push_back({0, 1});
    newData.push_back({pwm1period * pwm1.duty, 1});
    newData.push_back({pwm1period * pwm1.duty, 0});
    newData.push_back({pwm1period, 0});
    graphData->set(newData, true);

    plot->rescaleAxes();
    plot->replot();
}

void PwmOutputPlotController::setPlot(QCustomPlot* plot) {
    this->plot = plot;
    plot->xAxis->setLabel("Time [us]");
    graph = plot->addGraph();
}
