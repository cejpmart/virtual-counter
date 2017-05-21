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
    view->setChart(chart);

    currentSeriesLine = new QtCharts::QLineSeries();
    chart->addSeries(currentSeriesLine);

    axisX = new QtCharts::QValueAxis();
    axisX->setTitleText("Time [s]");
    chart->setAxisX(axisX);
    currentSeriesLine->attachAxis(axisX);

    axisY = new QtCharts::QValueAxis();
    //axisY->setTitleText("Value???");
    chart->setAxisY(axisY);
    currentSeriesLine->attachAxis(axisY);

    showSeries(Series::frequency);
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

            currentSeriesLine->append(timestamps[i] - minTime, data[i]);
        }
    }

    maxTime = timestamps[count - 1];
    axisX->setRange(minTime - minTime, maxTime - minTime);

    double min, max;
    getAppropriateAxisRange(minValue, maxValue, min, max);
    axisY->setRange(min, max);
}

void MeasurementPlotView::clear() {
    currentSeriesLine->clear();
    haveMinTime = false;
}

void MeasurementPlotView::showSeries(Series series) {
    this->displayedSeries = series;

    switch (series) {
    case Series::frequency:
        axisY->setTitleText("Frequency [Hz]");
        break;
    case Series::period:
        axisY->setTitleText("Period [s]");
        break;
    case Series::interval:
        axisY->setTitleText("Interval [s]");
        break;
    case Series::phase:
        axisY->setTitleText("Phase [°]");
        break;
    case Series::freqRatio:
        axisY->setTitleText("Frequency Ratio [-]");
        break;
    }

    this->clear();
}
