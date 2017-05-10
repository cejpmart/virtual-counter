#include "pwmoutputplotcontroller.h"

static void renderWaveform(double period_us, QVector<QCPGraphData>& vector, const PwmParameters& params) {
    double period = (1.0 / params.freq) * 1000000;

    double t = -period * params.phase / 360;
    while (t < period_us) {
        vector.push_back({t, 1});
        t += period * params.duty;
        vector.push_back({t, 1});
        vector.push_back({t, 0});
        t += period * (1 - params.duty);
        vector.push_back({t, 0});
    }
}

PwmOutputPlotController::PwmOutputPlotController()
{
}

void PwmOutputPlotController::redraw(const PwmParameters& pwm1, const PwmParameters& pwm2)
{
    if (!plot)
        return;

    double pwm1period_us = (pwm1.enabled) ? (1.0 / pwm1.freq) * 1000000 : 1e-9;
    double pwm2period_us = (pwm2.enabled) ? (1.0 / pwm2.freq) * 1000000 : 1e-9;

    double totalPeriod_us = std::max(pwm1period_us * 3, pwm2period_us * 3);

    if (pwm1.enabled) {
        QVector<QCPGraphData> newData;
        renderWaveform(totalPeriod_us, newData, pwm1);
        pwmGraphs[0]->data()->set(newData, true);
    }

    if (pwm2.enabled) {
        QVector<QCPGraphData> newData;
        renderWaveform(totalPeriod_us, newData, pwm2);
        pwmGraphs[1]->data()->set(newData, true);
    }

    pwmGraphs[0]->setVisible(pwm1.enabled);
    pwmGraphs[1]->setVisible(pwm2.enabled);

    if (totalPeriod_us > 1e-6)
        plot->xAxis->setRange(0, totalPeriod_us);

    plot->replot();
}

void PwmOutputPlotController::setPlot(QCustomPlot* plot) {
    this->plot = plot;

    plot->legend->setVisible(true);
    plot->xAxis->setLabel("Time [us]");
    plot->yAxis->setLabel("Signal");
    plot->yAxis->setRange(0, 2);
    plot->yAxis->setTicks(false);

    pwmGraphs[0] = plot->addGraph();
    pwmGraphs[0]->setName("PWM A [A3]");        // FIXME: port name based on board
    pwmGraphs[0]->addToLegend();

    pwmGraphs[1] = plot->addGraph();
    pwmGraphs[1]->setName("PWM B [A6]");        // FIXME: port name based on board
    pwmGraphs[1]->setPen(QPen(QColor(255, 0, 0)));
    pwmGraphs[1]->addToLegend();
}
