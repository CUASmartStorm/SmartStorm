#ifndef SENSORDASHBOARD_H
#define SENSORDASHBOARD_H

#include <QMainWindow>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QDateTimeEdit>
#include <QTimer>
#include <QStatusBar>
#include <QGroupBox>
#include <QScrollArea>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QMap>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QAreaSeries>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>
#include <QDateTime>
#include <QDebug>

struct SensorChart {
    QChart      *chart     = nullptr;
    QChartView  *chartView = nullptr;
    QLineSeries *series    = nullptr;
    QAreaSeries *area      = nullptr;
    QDateTimeAxis *axisX   = nullptr;
    QValueAxis    *axisY   = nullptr;
};

class SensorDashboard : public QMainWindow
{
    Q_OBJECT

public:
    SensorDashboard(QWidget *parent = nullptr);
    ~SensorDashboard();

private slots:
    void onFetchClicked();
    void onAutoRefreshToggled(bool checked);
    void onAutoRefreshTimeout();
    void onSensorListReceived(QNetworkReply *reply);

private:
    void setupUI();
    void fetchSensorList();
    void fetchAllSensors();
    void fetchSensorData(const QString &sensorId);
    void onDataReceived(const QString &sensorId, QNetworkReply *reply);
    void updateChart(const QString &sensorId, const QJsonArray &dataArray);
    SensorChart &getOrCreateChart(const QString &sensorId);
    void setStatus(const QString &message);
    QString friendlyName(const QString &sensorId);
    QString unitLabel(const QString &sensorId);
    QColor seriesColor(const QString &sensorId);
    QColor areaColor(const QString &sensorId);
    bool   floorAtZero(const QString &sensorId);

    // UI
    QWidget *centralWidget;
    QVBoxLayout *mainLayout;

    // Controls
    QGroupBox *controlGroup;
    QLabel *startLabel;
    QDateTimeEdit *startDateTimeEdit;
    QLabel *endLabel;
    QDateTimeEdit *endDateTimeEdit;
    QPushButton *fetchButton;
    QCheckBox *autoRefreshCheckBox;
    QLabel *countdownLabel;

    // Charts area
#ifdef SCROLLABLE_CHARTS
    QScrollArea *scrollArea;
#endif
    QWidget *chartsContainer;
    QVBoxLayout *chartsLayout;

    // One chart per sensor
    QMap<QString, SensorChart> sensorCharts;

    // Sensor list (populated dynamically)
    QStringList sensorIds;

    // Network
    QNetworkAccessManager *networkManager;
    QString apiUrl;
    int pendingRequests;

    // Auto-refresh
    QTimer *refreshTimer;
    QTimer *countdownTimer;
    int countdownSeconds;
};

#endif // SENSORDASHBOARD_H
