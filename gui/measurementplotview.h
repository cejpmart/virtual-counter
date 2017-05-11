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
    ratio,
};

class MeasurementPlotView : public QObject
{
    Q_OBJECT
public:
    explicit MeasurementPlotView(QtCharts::QChartView* view, QObject *parent = 0);

    void addDataPoints(Series series, const double* timestamps, const double* data, const double* errors, size_t count);

signals:

public slots:

private:
    QtCharts::QChart* chart;
    QtCharts::QValueAxis* axisX, * axisY;

    bool haveMinTime = false;
    double minTime, maxTime;
    double minValue, maxValue;

    QtCharts::QLineSeries* currentSeries;
};

#endif // MEASUREMENTPLOTVIEW_H
