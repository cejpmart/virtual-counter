#include "pwmoutputplotview.h"

#include <QtCharts/QValueAxis>

#include <QLayout>

static bool renderWaveform(double period_us, QtCharts::QXYSeries& series, const DgenOptions& params) {
    double period = (1.0 / params.freq) * 1000000;

    double t = -period * params.phase / 360;

    while (t > 0)
        t -= period;

    constexpr size_t MAX_ITERATIONS = 100;
    size_t i = 0;

    for (; t < period_us && i < MAX_ITERATIONS; i++) {
        series.append(t, 1);
        t += period * params.duty;
        series.append(t, 1);

        series.append(t, 0);
        t += period * (1 - params.duty);
        series.append(t, 0);
    }

    if (i >= MAX_ITERATIONS)
        return false;

    return true;
}

void PwmOutputPlotView::init(QtCharts::QChartView* view) {
    chart = new QtCharts::QChart();
    chart->setTheme(QtCharts::QChart::ChartThemeBlueNcs);

    chart->legend()->setVisible(true);

    pwmGraphs[0] = new QtCharts::QLineSeries();
    pwmGraphs[1] = new QtCharts::QLineSeries();

    chart->addSeries(pwmGraphs[0]);
    chart->addSeries(pwmGraphs[1]);

    // Must be done after adding to chart
    auto pen0 = pwmGraphs[0]->pen(); pen0.setStyle(Qt::DotLine); pwmGraphs[0]->setPen(pen0);
    auto pen1 = pwmGraphs[1]->pen(); pen1.setStyle(Qt::DotLine); pwmGraphs[1]->setPen(pen1);

    auto axisX = new QtCharts::QValueAxis();
    axisX->setTitleText("Time [us]");
    chart->setAxisX(axisX);
    pwmGraphs[0]->attachAxis(axisX);
    pwmGraphs[1]->attachAxis(axisX);

    /*auto axisY = new QtCharts::QValueAxis();
    axisY->setTitleText("Signal");
    axisY->setRange(0, 2);
    chart->setAxisY(axisY);
    pwmGraphs[0]->attachAxis(axisY);
    pwmGraphs[1]->attachAxis(axisY);*/

    //chart->setMargins(QMargins());
    view->setChart(chart);
}

void PwmOutputPlotView::redraw(const AllDgenOptions& options)
{
    if (!chart)
        return;

    double totalPeriod_us = 1e-6;

    for (size_t channel = 0; channel < options.size(); channel++) {
        double period_us = (options[channel].enabled) ? (1.0 / options[channel].freq) * 1000000 : 1e-9;

        totalPeriod_us = std::max(totalPeriod_us, period_us * 3);
    }

    for (size_t channel = 0; channel < options.size(); channel++) {
        if (options[channel].enabled) {
            pwmGraphs[channel]->clear();
            pwmGraphs[channel]->setVisible(renderWaveform(totalPeriod_us, *pwmGraphs[channel], options[channel]));
        }
        else {
            pwmGraphs[channel]->setVisible(false);
        }
    }

    if (totalPeriod_us > 1e-6)
        chart->axisX()->setRange(0, totalPeriod_us);
}

void PwmOutputPlotView::resetInstrument()
{
    pwmGraphs[0]->setName("PWM A [" + ipm.value("port.pwm_a") + "]");
    pwmGraphs[1]->setName("PWM B [" + ipm.value("port.pwm_b") + "]");
}
