
/////////////////////////////////////////////////////////////
// NOAAWEATHERFETCHER.H - Weather Fetcher Class Header
/////////////////////////////////////////////////////////////

#ifndef NOAAWEATHERFETCHER_H
#define NOAAWEATHERFETCHER_H

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <vector>
#include <iostream>

// Enum for different types of weather data
enum class datatype { ProbabilityofPrecipitation, Temperature, PrecipitationAmount, RelativeHumidity };

// Struct to hold weather prediction data point
struct WeatherData {
    QDateTime timestamp;  // Time of the prediction
    double value;         // Value of the measurement
};

class QChartView;

// Class for fetching weather data from NOAA API
class NOAAWeatherFetcher : public QObject {
    Q_OBJECT

public:
    NOAAWeatherFetcher(QObject* parent = nullptr);

    // Fetch weather prediction for specified location and data type
    QVector<WeatherData> getWeatherPrediction(int latitude, int longitude, datatype type);

private:
    QNetworkAccessManager* manager;  // Network manager for HTTP requests
};

// Helper function to calculate cumulative values over time
double calculateCumulativeValue(const QVector<WeatherData>& weatherData, int days);

#endif // NOAAWEATHERFETCHER_H

