
/////////////////////////////////////////////////////////////
// SMARTRAINHARVEST.H - Main Application Class Header
/////////////////////////////////////////////////////////////

#ifndef SMARTRAINHARVEST_H
#define SMARTRAINHARVEST_H

#include <QMainWindow>
#include "noaaweatherfetcher.h"
#include "chartcontainer.h"
#include "DistanceSensor.h"
#include "QTimer"
#include "QPushButton"
#include <QLabel>

QT_BEGIN_NAMESPACE
namespace Ui { class SmartRainHarvest; }
QT_END_NAMESPACE

class QChartView;

// Main application class - Smart rain harvesting system controller
class SmartRainHarvest : public QMainWindow
{
    Q_OBJECT

public:
    SmartRainHarvest(QWidget* parent = nullptr);
    ~SmartRainHarvest();

    // Data storage for charts (time series)
    QVector<WeatherData> cummulativeRain;  // Cumulative rain forecast
    QVector<WeatherData> depth;            // Water depth measurements
    QVector<WeatherData> openShut;         // Valve state history

    // System configuration parameters (adjustable)
    double barrelDepth = 100;              // Distance to bottom of barrel (cm)
    double forcastReleaseDepthThreshold = 50;         // Depth threshold for rain-forecast release (cm)
    double forcastReleaseThreshold = 50;    // Rain amount to trigger release (mm in 2 days)
    double overflowThreshold = 90;       // Depth threshold for overflow protection (cm)
    double overflowReleaseTargetDepth = 75;           // Target depth for overflow release (cm)
    double releaseTargetDepth = 5;                // Target depth for forecast-based release (cm)
    int checkWeatherInterval = 1;        // Weather check interval (seconds) (Originally 10)
    int checkReleaseInterval = 1;        // Release check interval (seconds) (Originally 10)
    bool checkDistance = true;             // Enable/disable distance sensor

    void startRelease();  // Begin water release process

private:
    const int VALVE_PIN = 18; // BCM pin 18 - Solenoid valve pin

    Ui::SmartRainHarvest* ui;
    NOAAWeatherFetcher fetcher;           // Weather API client
    int latitude = 97;                    // Grid latitude coordinate
    int longitude = 71;                   // Grid longitude coordinate

    // Chart containers for visualization
    ChartContainer* ProbnQuanChartContainer = new ChartContainer();          // Weather forecasts
    ChartContainer* CummulativeForcastChartContainer = new ChartContainer(); // Cumulative rain
    ChartContainer* WaterDepthChartContainer = new ChartContainer();         // Water depth
    ChartContainer* OpenShutChartContainer = new ChartContainer();           // Valve state

    DistanceSensor distancesensor;        // Ultrasonic sensor interface
    QTimer* ReleaseTimer = new QTimer();  // Timer for water release monitoring

    void shutTheValve();                  // Close valve function
    void openTheValve();                  // Open valve function

    bool inOverflowMode = false;                // Flag: overflow mode active
    bool valveOpen = false;                   // Current valve state (open/closed)

    // UI controls
    QPushButton* ManualOpenShut;          // Manual valve control button
    QLabel* DistanceLbl;                  // Distance display label

public slots:
    void onCheckTimer();                // Periodic weather check callback
    void onCheckDistance();             // Periodic distance check during release
    void onManualOpenShut();             // Manual valve toggle callback
};

#endif // SMARTRAINHARVEST_H


/////////////////////////////////////////////////////////////
