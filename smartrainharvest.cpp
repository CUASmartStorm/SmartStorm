
/////////////////////////////////////////////////////////////
// SMARTRAINHARVEST.CPP - Main Application Logic
/////////////////////////////////////////////////////////////

#include "smartrainharvest.h"
#include "ui_smartrainharvest.h"
#include "noaaweatherfetcher.h"
#include "chartcontainer.h"
#include <QMap>
#include <QTimer>
#include <DistanceSensor.h>
#include <QSplitter>
#include <QLabel>
#include <QPushButton>
#include <wiringPi.h>

// Constructor - Initialize the main window and all components
SmartRainHarvest::SmartRainHarvest(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::SmartRainHarvest)
{
    ui->setupUi(this);

    // Location coordinates (Washington DC area)
    //Latitude 38.9072° N
    //Longitude: 77.0369 W

    // Create vertical splitter for main layout
    QSplitter* splitter = new QSplitter(Qt::Vertical, this);

    // Create timer for periodic weather checks
    QTimer* CheckWeatherTimer = new QTimer();

    // Connect timers to their respective slots
    connect(CheckWeatherTimer, SIGNAL(timeout()), this, SLOT(onCheckTimer()));
    connect(ReleaseTimer, SIGNAL(timeout()), this, SLOT(onCheckDistance()));

    // Start weather check timer (interval defined in class)
    CheckWeatherTimer->start(checkWeatherInterval * 1000);

    // Setup UI layout with charts
    splitter->addWidget(ProbnQuanChartContainer->GetChartView());
    QSplitter* hsplitter = new QSplitter(Qt::Horizontal, splitter);
    hsplitter->addWidget(CummulativeForcastChartContainer->GetChartView());
    hsplitter->addWidget(WaterDepthChartContainer->GetChartView());
    splitter->addWidget(OpenShutChartContainer->GetChartView());

    // Create control panel with manual valve button and distance display
    QWidget* layoutWidget1 = new QWidget(hsplitter);
    QVBoxLayout* buttonslayout = new QVBoxLayout(layoutWidget1);
    ManualOpenShut = new QPushButton(splitter);
    buttonslayout->addWidget(ManualOpenShut);
    ManualOpenShut->setText("Open Valve");
    DistanceLbl = new QLabel(splitter);
    buttonslayout->addWidget(DistanceLbl);
    connect(ManualOpenShut, SIGNAL(clicked()), this, SLOT(onManualOpenShut()));

    // Set stretch factors for splitters (relative sizing)
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    splitter->setStretchFactor(2, 1);
    hsplitter->setStretchFactor(0, 3);
    hsplitter->setStretchFactor(1, 3);
    hsplitter->setStretchFactor(2, 1);

    // Set the splitter as the central widget
    setCentralWidget(splitter);

    // Initialize distance sensor and valve control pin
    distancesensor.initialize();
    pinMode(VALVE_PIN, OUTPUT);  // GPIO 18 for valve control

    // Run initial weather check
    onCheckTimer();
}

// Destructor
SmartRainHarvest::~SmartRainHarvest()
{
    delete ui;
}

// Periodic timer callback - Check weather and update system state
void SmartRainHarvest::onCheckTimer()
{
    qDebug() << "Checking weather!";

    // Container for multiple weather data series
    QMap<QString, QVector<WeatherData>> precip_data;

    // Fetch precipitation amount forecast
    QVector<WeatherData> rainamountdata = fetcher.getWeatherPrediction(latitude, longitude, datatype::PrecipitationAmount);
    precip_data["Precipitation [mm]"] = rainamountdata;

    // Fetch precipitation probability forecast
    QVector<WeatherData> rainprobdata = fetcher.getWeatherPrediction(latitude, longitude, datatype::ProbabilityofPrecipitation);
    precip_data["Precipitation probability (%)"] = rainprobdata;

    // Fetch temperature forecast
    QVector<WeatherData> tempdata = fetcher.getWeatherPrediction(latitude, longitude, datatype::Temperature);
    precip_data["Temperature (<sup>o</sup>C)"] = tempdata;

    // Plot all weather data on the multi-line chart
    ProbnQuanChartContainer->plotWeatherDataMap(precip_data);
    ProbnQuanChartContainer->GetChartView()->setRenderHint(QPainter::Antialiasing);

    // Maintain rolling window of 30 cumulative rain data points
    if (cummulativeRain.count() > 30) cummulativeRain.removeFirst();

    // Calculate and store 2-day cumulative rain forecast
    cummulativeRain.append({ QDateTime::currentDateTime(),calculateCumulativeValue(rainamountdata,2) });
    CummulativeForcastChartContainer->plotWeatherData(cummulativeRain, "Cummulative rain forecast [mm]");

    // Check distance sensor if enabled
    double distance;
    if (checkDistance)
    {
        if (!valveOpen) // The depth is found using the release timer if in release mode.
        {
            distance = barrelDepth - distancesensor.getDistance();
            // Cap water depth.
            if (distance > barrelDepth) distance = barrelDepth;
            else if (distance < 0) distance = 0;

            qDebug() << "Distance = " << distance << " cm";
            DistanceLbl->setText(QString::number(distance));

            // Maintain rolling window of 30 depth measurements
            if (depth.count() > 30) depth.removeFirst();
            depth.append({ QDateTime::currentDateTime(), distance });
            WaterDepthChartContainer->plotWeatherData(depth, "Water Depth (cm)");
        }

        // Decision logic: Open valve if depth is high AND rain is forecast
        if (depth.last().value > forcastReleaseDepthThreshold && cummulativeRain.last().value > forcastReleaseThreshold)
        {
            inOverflowMode = false;
            startRelease();
        }

        // Decision logic: Open valve if depth exceeds bypass threshold (overflow protection)
        if (depth.last().value > overflowThreshold)
        {
            inOverflowMode = true;
            startRelease();
        }
    }

    // Update valve state chart (maintain rolling window of 100 points)
    if (openShut.count() > 100) openShut.removeFirst();
    openShut.append({ QDateTime::currentDateTime(), double(int(valveOpen)) });
    OpenShutChartContainer->plotWeatherData(openShut, "Valve State (on/off)");

    // Update manual button text based on current state
    if (!valveOpen)
        ManualOpenShut->setText("Open the Valve");
    else
        ManualOpenShut->setText("Shut the Valve");
}

// Start the water release timer
void SmartRainHarvest::startRelease()
{
    ReleaseTimer->start(checkReleaseInterval * 1000);  // Check for target depth on a timer.
}

// Distance check callback during water release
void SmartRainHarvest::onCheckDistance()
{
    // Measure current water depth
    double distance = barrelDepth - distancesensor.getDistance();
    // Cap water depth.
    if (distance > barrelDepth) distance = barrelDepth;
    else if (distance < 0) distance = 0;

    if (depth.count() > 30) depth.removeFirst();
    depth.append({ QDateTime::currentDateTime(), distance });
    WaterDepthChartContainer->plotWeatherData(depth, "Water Depth (cm)");

    // Update valve state chart
    if (openShut.count() > 100) openShut.removeFirst();
    openShut.append({ QDateTime::currentDateTime(), double(valveOpen) });
    OpenShutChartContainer->plotWeatherData(openShut, "Valve State (on/off");

    // Decision logic: Stop release when target depth is reached
    if (depth.last().value < (inOverflowMode ? overflowReleaseTargetDepth : releaseTargetDepth))
    {
        ReleaseTimer->stop();
        shutTheValve();
        valveOpen = false;
    }
    else
    {
        // Continue releasing water
        openTheValve();
        valveOpen = true;
    }
}

// Open the solenoid valve (GPIO HIGH)
void SmartRainHarvest::openTheValve() {
    digitalWrite(VALVE_PIN, HIGH);
    qDebug() << "The valve is now open";
}

// Close the solenoid valve (GPIO LOW)
void SmartRainHarvest::shutTheValve() {
    digitalWrite(VALVE_PIN, LOW);
    qDebug() << "The valve is now shut";
}

// Manual valve control button callback
void SmartRainHarvest::onManualOpenShut()
{
    // Toggle valve state
    if (valveOpen)
    {
        shutTheValve();
        valveOpen = false;
    }
    else
    {
        openTheValve();
        valveOpen = true;
    }

    // Update button text
    if (!valveOpen)
        ManualOpenShut->setText("Open the Valve");
    else
        ManualOpenShut->setText("Shut the Valve");

    // Update valve state chart
    if (openShut.count() > 100) openShut.removeFirst();
    openShut.append({ QDateTime::currentDateTime(), double(int(valveOpen)) });
    OpenShutChartContainer->plotWeatherData(openShut, "Valve State (on/off)");
}


