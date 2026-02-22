
/////////////////////////////////////////////////////////////
// CHARTCONTAINER.H - Chart Container Class Header
/////////////////////////////////////////////////////////////

#ifndef CHARTCONTAINER_H
#define CHARTCONTAINER_H
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QDateTimeAxis>
#include <QMap>

#include "noaaweatherfetcher.h"

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
using namespace QtCharts;
#endif

// Class for managing chart visualization of weather data
class ChartContainer
{
public:
    ChartContainer();

    // Plot a single weather data series
    void plotWeatherData(const QVector<WeatherData>& weatherData, const QString& yAxisTitle);

    // Plot multiple weather data series on the same chart
    void plotWeatherDataMap(const QMap<QString, QVector<WeatherData>>& weatherDataMap);

    // Getter for the chart view widget
    QChartView* GetChartView() { return chartview; }

private:
    QChart* chart = new QChart();                              // The chart object
    QChartView* chartview = new QChartView();                  // Widget to display the chart
    void removeAllAxes();                                      // Helper to clear all axes
    QVector<QColor> colors;                                    // Color palette for series
};

#endif // CHARTCONTAINER_H


