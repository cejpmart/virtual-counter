#ifndef MEASUREMENTPLOTVIEW_H
#define MEASUREMENTPLOTVIEW_H

#include <QObject>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

enum class Series {
    frequency,
    period,
    interval,
    phase,
    freqRatio,
};

class MeasurementPlotView : public QObject
{
    Q_OBJECT
public:
    explicit MeasurementPlotView(QtCharts::QChartView* view, QObject *parent = 0);

    void addDataPoints(Series series, const double* timestamps, const double* data, const double* errors, size_t count);

    void clear();

    void showSeries(Series series);

signals:

public slots:

private:
    QtCharts::QChart* chart;
    QtCharts::QValueAxis* axisX, * axisY;

    bool haveMinTime = false;
    double minTime, maxTime;
    double minValue, maxValue;

    Series displayedSeries;
    QtCharts::QLineSeries* currentSeriesLine;
};

#endif // MEASUREMENTPLOTVIEW_H
