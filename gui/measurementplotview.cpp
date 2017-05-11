#include "measurementplotview.h"

#include <cmath>

static void getAppropriateAxisRange(double min, double max, double& min_out, double& max_out)
{
    double delta = max - min;

    if (delta > 1e-7) {
        double del_log = floor(log10(max - min));
        double magn = pow(10, del_log);

        min_out = floor(min / magn) * magn;
        max_out = ceil(max / magn) * magn;
    }
    else {
        min_out = floor(min);
        max_out = ceil(max + 1);
    }
}

MeasurementPlotView::MeasurementPlotView(QtCharts::QChartView* view, QObject *parent) : QObject(parent)
{
    chart = new QtCharts::QChart();

    chart->createDefaultAxes();

    currentSeries = new QtCharts::QLineSeries();

    chart->addSeries(currentSeries);

    axisX = new QtCharts::QValueAxis();
    axisX->setTitleText("Time [s]");
    chart->setAxisX(axisX);
    currentSeries->attachAxis(axisX);

    axisY = new QtCharts::QValueAxis();
    axisY->setTitleText("Value???");
    chart->setAxisY(axisY);
    currentSeries->attachAxis(axisY);

    view->setChart(chart);
}

void MeasurementPlotView::addDataPoints(Series series, const double* timestamps, const double* data, const double* errors, size_t count) {
    if (!haveMinTime && count) {
        minTime = timestamps[0];
        maxTime = timestamps[0];
        minValue = data[0];
        maxValue = data[0];
        haveMinTime = true;
    }

    if (series == Series::frequency) {
        for (size_t i = 0; i < count; i++) {
            if (minValue > data[i])
                minValue = data[i];

            if (maxValue < data[i])
                maxValue = data[i];

            currentSeries->append(timestamps[i], data[i]);
        }
    }

    maxTime = timestamps[count - 1];
    axisX->setRange(minTime, maxTime);

    double min, max;
    getAppropriateAxisRange(minValue, maxValue, min, max);
    axisY->setRange(min, max);
}
