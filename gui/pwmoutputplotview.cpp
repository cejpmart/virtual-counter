#include "pwmoutputplotview.h"

#include <QtCharts/QValueAxis>

#include <QLayout>

static void renderWaveform(double period_us, QtCharts::QXYSeries& series, const PwmParameters& params) {
    double period = (1.0 / params.freq) * 1000000;

    double t = -period * params.phase / 360;

    while (t > 0)
        t -= period;

    while (t < period_us) {
        series.append(t, 1);
        t += period * params.duty;
        series.append(t, 1);

        series.append(t, 0);
        t += period * (1 - params.duty);
        series.append(t, 0);
    }
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

void PwmOutputPlotView::redraw(const PwmParameters& pwm1, const PwmParameters& pwm2)
{
    if (!chart)
        return;

    double pwm1period_us = (pwm1.enabled) ? (1.0 / pwm1.freq) * 1000000 : 1e-9;
    double pwm2period_us = (pwm2.enabled) ? (1.0 / pwm2.freq) * 1000000 : 1e-9;

    double totalPeriod_us = std::max(pwm1period_us * 3, pwm2period_us * 3);

    if (pwm1.enabled) {
        pwmGraphs[0]->clear();
        renderWaveform(totalPeriod_us, *pwmGraphs[0], pwm1);
    }

    if (pwm2.enabled) {
        pwmGraphs[1]->clear();
        renderWaveform(totalPeriod_us, *pwmGraphs[1], pwm2);
    }

    pwmGraphs[0]->setVisible(pwm1.enabled);
    pwmGraphs[1]->setVisible(pwm2.enabled);

    if (totalPeriod_us > 1e-6)
        chart->axisX()->setRange(0, totalPeriod_us);
}

void PwmOutputPlotView::resetInstrument()
{
    pwmGraphs[0]->setName("PWM A [" + ipm.value("port.pwm_a") + "]");
    pwmGraphs[1]->setName("PWM B [" + ipm.value("port.pwm_b") + "]");
}
