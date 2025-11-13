
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
    connect(CheckWeatherTimer, SIGNAL(timeout()), this, SLOT(on_Check_Timer()));
    connect(ReleaseTimer, SIGNAL(timeout()), this, SLOT(on_Check_Distance()));

    // Start weather check timer (interval defined in class)
    CheckWeatherTimer->start(Check_Weather_Interval * 1000);

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
    connect(ManualOpenShut, SIGNAL(clicked()), this, SLOT(on_ManualOpenShut()));

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
    pinMode(18, OUTPUT);  // GPIO 18 for valve control

    // Run initial weather check
    on_Check_Timer();
}

// Destructor
SmartRainHarvest::~SmartRainHarvest()
{
    delete ui;
}

// Periodic timer callback - Check weather and update system state
void SmartRainHarvest::on_Check_Timer()
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
    if (cummulativerain.count() > 30) cummulativerain.removeFirst();

    // Calculate and store 2-day cumulative rain forecast
    cummulativerain.append({ QDateTime::currentDateTime(),calculateCumulativeValue(rainamountdata,2) });
    CummulativeForcastChartContainer->plotWeatherData(cummulativerain, "Cummulative rain forecast [mm]");

    // Check distance sensor if enabled
    double distance;
    if (checkdistance)
    {
        distance = distancesensor.getDistance();
        qDebug() << "Distance = " << distance << " cm";
        DistanceLbl->setText(QString::number(distance));

        // Maintain rolling window of 30 depth measurements
        if (depth.count() > 30) depth.removeFirst();
        depth.append({ QDateTime::currentDateTime(), max_distance - distance });
        WaterDepthChartContainer->plotWeatherData(depth, "Water Depth (cm)");

        // Decision logic: Open valve if depth is high AND rain is forecast
        if (depth.last().value > waterdepthcriteria && cummulativerain.last().value > cummulativeraincriteria)
        {
            overflow = false;
            StartRelease();
        }

        // Decision logic: Open valve if depth exceeds bypass threshold (overflow protection)
        if (depth.last().value > bypassdepthcriteria)
        {
            overflow = true;
            StartRelease();
        }
    }

    // Update valve state chart (maintain rolling window of 100 points)
    if (openshut.count() > 100) openshut.removeFirst();
    openshut.append({ QDateTime::currentDateTime(), double(int(state)) });
    OpenShutChartContainer->plotWeatherData(openshut, "Valve State (on/off)");

    // Update manual button text based on current state
    if (!state)
        ManualOpenShut->setText("Open the Valve");
    else
        ManualOpenShut->setText("Shut the Valve");
}

// Start the water release timer
void SmartRainHarvest::StartRelease()
{
    ReleaseTimer->start(10000);  // Check every 10 seconds
}

// Distance check callback during water release
void SmartRainHarvest::on_Check_Distance()
{
    // Measure current water depth
    double distance = distancesensor.getDistance();
    if (depth.count() > 30) depth.removeFirst();
    depth.append({ QDateTime::currentDateTime(), max_distance - distance });
    WaterDepthChartContainer->plotWeatherData(depth, "Water Depth (cm)");

    // Update valve state chart
    if (openshut.count() > 100) openshut.removeFirst();
    openshut.append({ QDateTime::currentDateTime(), double(state) });
    OpenShutChartContainer->plotWeatherData(openshut, "Valve State (on/off");

    // Decision logic: Stop release when target depth is reached
    if (depth.last().value < (overflow ? depthtoreleaseto : minumumdepth))
    {
        ReleaseTimer->stop();
        ShutTheValve();
        state = false;
    }
    else
    {
        // Continue releasing water
        OpenTheValve();
        state = true;
    }
}

// Open the solenoid valve (GPIO HIGH)
void SmartRainHarvest::OpenTheValve() {
    digitalWrite(18, HIGH);
    qDebug() << "The valve is now open";
}

// Close the solenoid valve (GPIO LOW)
void SmartRainHarvest::ShutTheValve() {
    digitalWrite(18, LOW);
    qDebug() << "The valve is now shut";
}

// Manual valve control button callback
void SmartRainHarvest::on_ManualOpenShut()
{
    // Toggle valve state
    if (state)
    {
        ShutTheValve();
        state = false;
    }
    else
    {
        OpenTheValve();
        state = true;
    }

    // Update button text
    if (!state)
        ManualOpenShut->setText("Open the Valve");
    else
        ManualOpenShut->setText("Shut the Valve");

    // Update valve state chart
    if (openshut.count() > 100) openshut.removeFirst();
    openshut.append({ QDateTime::currentDateTime(), double(int(state)) });
    OpenShutChartContainer->plotWeatherData(openshut, "Valve State (on/off)");
}


