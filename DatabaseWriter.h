/////////////////////////////////////////////////////////////
// DATABASEWRITER.H - Database Writer Class Header
/////////////////////////////////////////////////////////////

#ifndef DATABASEWRITER_H
#define DATABASEWRITER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <QUrl>
#include <QDebug>
#include <QDateTime>
#include <QVector>

#include "noaaweatherfetcher.h"

class DatabaseWriter : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseWriter(QObject *parent = nullptr);

    // Generic: send a single reading to the API
    void sendReading(const QString &sensorId, double value,
                     const QString &unit, const QDateTime &timestamp = QDateTime());

    // Weather forecasts: send an entire forecast array
    // sensorId examples: "precip_amount", "precip_prob", "temperature"
    void sendWeatherData(const QString &sensorId, const QString &unit,
                         const QVector<WeatherData> &weatherData);

    // Convenience methods for specific data types
    void sendDepthReading(double depthCm);
    void sendValveState(bool open);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *manager;
    QUrl apiUrl;

    void postJson(const QJsonObject &json);
    int failCount = 0;
};

#endif // DATABASEWRITER_H
