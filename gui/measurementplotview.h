#ifndef MEASUREMENTPLOTVIEW_H
#define MEASUREMENTPLOTVIEW_H

#include <QObject>

#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QValueAxis>
#include <QtCharts/QXYSeries>

enum class Series {
    frequency,
    period,
    interval,
    phase,
    freqRatio,
    dutyCycle,
};

class MeasurementPlotView : public QObject
{
    Q_OBJECT
public:
    explicit MeasurementPlotView(QtCharts::QChartView* view, QObject *parent = 0);

    void addDataPoints(Series series, const double* timestamps, const double* data, const double* errors, size_t count);
    void clear();
    void savePNG(QString path);
    void saveSeries(QString path);
    void setWindowSeconds(int seconds = -1);
    void showSeries(Series series);

signals:

public slots:

private:
    QtCharts::QChartView* view;
    QtCharts::QChart* chart;
    QtCharts::QValueAxis* axisX, * axisY;

    bool haveMinTime = false;
    double minTime, maxTime;
    double minValue, maxValue;
    int windowSeconds = -1;

    Series displayedSeries;
    QtCharts::QXYSeries* currentSeriesLine;
};

#endif // MEASUREMENTPLOTVIEW_H
