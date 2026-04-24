/////////////////////////////////////////////////////////////
// SMARTRAINHARVEST.H - Main Application Class Header
/////////////////////////////////////////////////////////////

#ifndef SMARTRAINHARVEST_H
#define SMARTRAINHARVEST_H

#include <QMainWindow>
#include "noaaweatherfetcher.h"
#include "chartcontainer.h"
#include "DistanceSensor.h"
#include "MoistureSensor.h"
#include "DatabaseWriter.h"
#include <QTimer>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QGroupBox>
#include <QFrame>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
//using namespace QtCharts;
#endif

QT_BEGIN_NAMESPACE
namespace Ui { class SmartRainHarvest; }
QT_END_NAMESPACE

// ── System operating states ────────────────────────────────
enum class SystemState {
    Monitoring,
    Releasing
};

enum class ReleaseReason {
    None,
    Overflow,
    Forecast,
    Dry,
    DryAndForecast
};

class SmartRainHarvest : public QMainWindow
{
    Q_OBJECT

public:
    SmartRainHarvest(QWidget *parent = nullptr);
    ~SmartRainHarvest();

    // ── Tunable parameters ─────────────────────────────────

    // Physical distance from the ultrasonic sensor (mounted at top)
    // to the bottom of the barrel. All depth readings are calculated
    // as: depth = barrelDepth − sensorReading
    double barrelDepth = 137.16;                        // cm

    // Forecast-based release triggers when BOTH conditions are met:
    //   1. Current water depth  >  forecastReleaseDepthThreshold
    //   2. 2-day cumulative rain >  forecastReleaseThreshold
    // The system then drains down to forecastTargetDepth.
    double forecastThreshold      = 15;       // mm  — min predicted rain (2-day sum)
    double moistureThreshold      = 20;      // %  — minimum soil moisture to consider draining

    // Overflow protection triggers when:
    //   Current water depth  >  overflowThreshold
    // This fires regardless of the rain forecast (safety mechanism).
    // The system drains down to overflowTargetDepth.
    double overflowThreshold     = 124;               // cm  — emergency release trigger
    double emptyThreshold     = 13;                 // cm  — point at which the barrel is empty.
    int monitoringInterval  = 3600;                    // Interval during closed valve mode (seconds)
    int releaseInterval     = 300;                     // Interval during open valve mode (seconds)
    bool sensorEnabled      = true;

private slots:
    void onMonitoringTick();
    void onReleaseTick();
    void onManualOpenShut();
    void onAutoControlToggled(bool checked);
    bool checkIfShouldRelease();

private:
    // ── State ──────────────────────────────────────────────
    SystemState   state         = SystemState::Monitoring;
    ReleaseReason releaseReason = ReleaseReason::None;
    bool          valveOpen     = false;
    bool          autoControl   = true;
    double        lastDepth     = 0;
    double        lastMoisture  = 0;
    double        lastCumRain   = 0;

    // ── Hardware ───────────────────────────────────────────
    //const int VALVE_PIN = 18;
    static constexpr int VALVE_OPEN_PIN = 18;
    static constexpr int VALVE_CLOSE_PIN = 23;
    static constexpr int VALVE_PULSE_MS = 5000;

    DistanceSensor distanceSensor;
    void openValve();
    void shutValve();

    MoistureSensor moistureSensor;

    // ── Depth measurement ──────────────────────────────────
    double measureDepth();
    int sensorFailCount = 0;           // Consecutive failed readings
    static const int MAX_SENSOR_FAILS = 3;  // Show error after this many

    // ── Soil moisture measurement ─────────────────────────-
    double measureMoisture();



    // ── State transitions ──────────────────────────────────
    void enterReleaseMode();
    void enterMonitoringMode();



    // ── Timers ─────────────────────────────────────────────
    QTimer *monitoringTimer;
    QTimer *releaseTimer;

    // ── Weather ────────────────────────────────────────────
    NOAAWeatherFetcher fetcher;
    int gridX = 97;
    int gridY = 71;

    // ── Data history ───────────────────────────────────────
    QVector<WeatherData> cumulativeRainHistory;
    QVector<WeatherData> depthHistory;
    QVector<WeatherData> moistureHistory;
    QVector<WeatherData> valveHistory;
    static const int MAX_HISTORY = 100;

    void recordDepth(double depth);
    void recordMoisture(double moisture);
    void recordValveState();

    // ── UI setup ───────────────────────────────────────────
    Ui::SmartRainHarvest *ui;
    void setupDashboard();
    void updateInfoPanels();
    void updateModeIndicator();
    void updateValveButton();

    // ── Charts ─────────────────────────────────────────────
    ChartContainer *weatherChart    = new ChartContainer();
    ChartContainer *cumulativeChart = new ChartContainer();
    ChartContainer *depthChart      = new ChartContainer();
    ChartContainer *moistureChart   = new ChartContainer();
    ChartContainer *valveChart      = new ChartContainer();

    // ── Info panel labels ──────────────────────────────────
    QLabel *depthValueLabel;
    QLabel *depthUnitLabel;
    QLabel *sensorStatusLabel;
    QProgressBar *depthBar;

    QLabel *moistureValueLabel;
    QLabel *moistureUnitLabel;
    QLabel *moistureStatusLabel;
    QProgressBar *moistureBar;

    QLabel *rainValueLabel;
    QLabel *rainUnitLabel;
    QProgressBar *rainBar;

    QLabel *modeValueLabel;
    QLabel *modeReasonLabel;
    QFrame *modeIndicator;

    QLabel *valveValueLabel;
    QFrame *valveIndicator;

    // ── Threshold labels ───────────────────────────────────
    QLabel *threshOverflowLabel;
    QLabel *threshEmptyLabel;
    QLabel *threshForecastRainLabel;
    QLabel *threshmoistureLabel;

    // ── Controls ───────────────────────────────────────────
    QPushButton *manualButton;
    QCheckBox   *autoControlCheckBox;

    // ── Database ───────────────────────────────────────────
    DatabaseWriter dbWriter;
};

#endif // SMARTRAINHARVEST_H
