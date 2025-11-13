

/////////////////////////////////////////////////////////////
// NOAAWEATHERFETCHER.CPP - NOAA Weather API Client
/////////////////////////////////////////////////////////////

#include "noaaweatherfetcher.h"
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QJsonDocument>
#include <QEventLoop>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <vector>
#include <iostream>

// Constructor - initialize network manager
NOAAWeatherFetcher::NOAAWeatherFetcher(QObject* parent) : QObject(parent) {
    manager = new QNetworkAccessManager(this);
}

// Fetch weather prediction data from NOAA API
QVector<WeatherData> NOAAWeatherFetcher::getWeatherPrediction(int latitude, int longitude, datatype type) {
    QVector<WeatherData> weatherData;

    // Construct NOAA API URL for the specified grid coordinates
    QString url;
    url = QString("https://api.weather.gov/gridpoints/LWX/%1,%2").arg(latitude).arg(longitude);
    qDebug() << url;

    // Create HTTP GET request
    QNetworkRequest request((QUrl(url)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Map data type enum to NOAA API field name
    QString DataType;
    switch (type) {
    case datatype::PrecipitationAmount:
        DataType = "quantitativePrecipitation";
        break;
    case datatype::ProbabilityofPrecipitation:
        DataType = "probabilityOfPrecipitation";
        break;
    case datatype::RelativeHumidity:
        DataType = "relativeHumidity";
        break;
    case datatype::Temperature:
        DataType = "temperature";
        break;
    }
    qDebug() << DataType;

    // Send the request
    QNetworkReply* reply = manager->get(request);

    // Wait for the reply to finish (blocking approach for simplicity)
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    // Process the response if successful
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(response);
        QJsonObject jsonObject = jsonResponse.object();
        QJsonObject properties = jsonObject["properties"].toObject();
        qDebug() << properties[DataType];

        // Extract the time series values array
        QJsonArray periods = properties[DataType].toObject()["values"].toArray();
        qDebug() << periods;

        // Parse each time period's data
        for (const auto& period : periods) {
            QJsonObject obj = period.toObject();

            // Extract timestamp and value
            qDebug() << obj["validTime"].toString().split("+")[0];
            QDateTime time = QDateTime::fromString(obj["validTime"].toString().split("+")[0], "yyyy-MM-ddTHH:mm:ss");
            qDebug() << time;
            double value = obj["value"].toDouble();

            weatherData.push_back({ time, value });
        }
    }
    else {
        qWarning() << "Error fetching weather data:" << reply->errorString();
    }

    reply->deleteLater();
    return weatherData;
}

// Calculate cumulative value over a specified number of days
double calculateCumulativeValue(const QVector<WeatherData>& weatherData, int days) {
    if (weatherData.isEmpty()) return 0.0;

    // Define time window
    QDateTime startTime = weatherData.first().timestamp;
    QDateTime endTime = startTime.addDays(days);

    // Sum all values within the time window
    double cumulativeValue = 0.0;
    for (const auto& data : weatherData) {
        if (data.timestamp <= endTime) {
            cumulativeValue += data.value;
        }
        else {
            break;
        }
    }

    return cumulativeValue;
}


