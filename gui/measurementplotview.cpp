#include "measurementplotview.h"

#include <cmath>

#include <QTextStream>

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

static QString getSeriesTitle(Series series)
{
    switch (series) {
    case Series::frequency: return "Frequency [Hz]";
    case Series::period:    return "Period [s]";
    case Series::interval:  return "Interval [s]";
    case Series::phase:     return "Phase [°]";
    case Series::freqRatio: return "Frequency Ratio [-]";
    case Series::dutyCycle: return "Duty Cycle [%]";
    default:                return "Unknown";
    }
}

MeasurementPlotView::MeasurementPlotView(QtCharts::QChartView* view, QObject *parent) : QObject(parent), view(view)
{
    chart = new QtCharts::QChart();
    chart->createDefaultAxes();
    chart->legend()->setVisible(false);
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
    if (series != displayedSeries)
        return;

    if (!haveMinTime && count) {
        minTime = timestamps[0];
        maxTime = timestamps[0];
        minValue = data[0];
        maxValue = data[0];
        haveMinTime = true;
    }

    if (series == Series::dutyCycle) {
        for (size_t i = 0; i < count; i++) {
            currentSeriesLine->append(timestamps[i] - minTime, data[i]);
        }
    }
    else {
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

    if (displayedSeries == Series::dutyCycle) {
        axisY->setRange(0, 100);
    }
    else {
        double min, max;
        getAppropriateAxisRange(minValue, maxValue, min, max);
        axisY->setRange(min, max);
    }
}

void MeasurementPlotView::clear() {
    currentSeriesLine->clear();
    haveMinTime = false;
}

#include <QtPrintSupport/QPrinter>

void MeasurementPlotView::savePNG(QString path) {
    int w = view->width();
    int h = view->height();
    double scale = 2;

    QImage img(w * scale, h * scale, QImage::Format_RGB888);

    QPainter painter;
    painter.begin(&img);
    //painter.scale(1, 1);
    view->render(&painter);
    painter.end();

    img.save(path);
}

void MeasurementPlotView::saveSeries(QString path) {
    QFile outputFile(path);

    if (outputFile.open(QIODevice::WriteOnly)) {
        QTextStream out(&outputFile);
        out << "Time [s]," << getSeriesTitle(displayedSeries) << "\n";

        auto points = currentSeriesLine->points();

        for (const auto& point : points) {
            out << QString::asprintf("%.9f,%.9f\n", point.x(), point.y());
        }
    }
}

void MeasurementPlotView::showSeries(Series series) {
    this->displayedSeries = series;

    axisY->setTitleText(getSeriesTitle(series));

    this->clear();
}
