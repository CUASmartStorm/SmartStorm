/////////////////////////////////////////////////////////////
// SMARTRAINHARVEST.CPP - Main Application Logic
//
//  Two-state architecture:
//    MONITORING  — slow timer, fetches weather, measures depth,
//                  evaluates release rules
//    RELEASING   — fast timer, measures depth, checks if target
//                  has been reached, shuts valve when done
/////////////////////////////////////////////////////////////

#include "smartrainharvest.h"
#include "ui_smartrainharvest.h"
#include <QMap>
#include <QSplitter>
#include <QFont>
#ifdef RasPi
#include <wiringPi.h>
#endif

// ================================================================
//  Constructor / Destructor
// ================================================================

SmartRainHarvest::SmartRainHarvest(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::SmartRainHarvest)
{



    ui->setupUi(this);

    // Timers
    monitoringTimer = new QTimer(this);
    releaseTimer    = new QTimer(this);
    connect(monitoringTimer, &QTimer::timeout,
            this, &SmartRainHarvest::onMonitoringTick);
    connect(releaseTimer, &QTimer::timeout,
            this, &SmartRainHarvest::onReleaseTick);

    setupDashboard();

    // Hardware
    distanceSensor.initialize();
    moistureSensor.initialize();
#ifdef RasPi
    pinMode(VALVE_OPEN_PIN, OUTPUT);
    pinMode(VALVE_CLOSE_PIN, OUTPUT);
#endif



    shutValve();
    enterMonitoringMode();
    QTimer::singleShot(0, this, &SmartRainHarvest::onMonitoringTick);


    //onMonitoringTick();
}

SmartRainHarvest::~SmartRainHarvest()
{
    delete ui;
}

// ================================================================
//  UI Setup
// ================================================================

void SmartRainHarvest::setupDashboard()
{
    setWindowTitle("SmartRainHarvest");
    resize(1400, 900);

    // ── Global stylesheet ──────────────────────────────────
    setStyleSheet(R"(
        QMainWindow {
            background-color: #1a1d23;
        }
        QSplitter::handle {
            background-color: #2d3139;
        }
    )");

    // ── Helper: create an info card ────────────────────────
    auto makeCard = [](const QString &title, QWidget *parent) -> QGroupBox* {
        QGroupBox *box = new QGroupBox(title, parent);
        box->setStyleSheet(R"(
            QGroupBox {
                font-size: 11px;
                font-weight: bold;
                color: #78909c;
                background-color: #21252b;
                border: 1px solid #2d3139;
                border-radius: 8px;
                margin-top: 10px;
                padding-top: 10px;
                padding-left: 10px;
                padding-right: 10px;
                padding-bottom: 6px;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 12px;
                padding: 0 6px;
            }
            QLabel {
                color: #cfd8dc;
                background: transparent;
                border: none;
            }
        )");
        return box;
    };

    // ── Helper: big value label ────────────────────────────
    auto makeBigLabel = [](const QString &text) -> QLabel* {
        QLabel *lbl = new QLabel(text);
        QFont f;
        f.setPixelSize(28);
        f.setBold(true);
        lbl->setFont(f);
        lbl->setStyleSheet("color: #eceff1; background: transparent;");
        return lbl;
    };

    // ── Helper: small label ────────────────────────────────
    auto makeSmallLabel = [](const QString &text) -> QLabel* {
        QLabel *lbl = new QLabel(text);
        lbl->setStyleSheet("color: #607d8b; font-size: 11px; background: transparent;");
        return lbl;
    };

    // ── Helper: progress bar ───────────────────────────────
    auto makeBar = [](const QString &color) -> QProgressBar* {
        QProgressBar *bar = new QProgressBar();
        bar->setRange(0, 100);
        bar->setValue(0);
        bar->setTextVisible(false);
        bar->setFixedHeight(6);
        bar->setStyleSheet(QString(R"(
            QProgressBar {
                background-color: #2d3139;
                border: none;
                border-radius: 3px;
            }
            QProgressBar::chunk {
                background-color: %1;
                border-radius: 3px;
            }
        )").arg(color));
        return bar;
    };

    // ── Helper: indicator dot ──────────────────────────────
    auto makeDot = [](const QString &color) -> QFrame* {
        QFrame *dot = new QFrame();
        dot->setFixedSize(14, 14);
        dot->setStyleSheet(QString(
                               "background-color: %1; border-radius: 7px; border: none;"
                               ).arg(color));
        return dot;
    };

    // ════════════════════════════════════════════════════════
    //  Build the info panel (left column)
    // ════════════════════════════════════════════════════════

    QWidget *infoPanel = new QWidget();
    infoPanel->setFixedWidth(260);
    infoPanel->setStyleSheet("background-color: #1a1d23;");
    QVBoxLayout *infoLayout = new QVBoxLayout(infoPanel);
    infoLayout->setContentsMargins(8, 8, 8, 8);
    infoLayout->setSpacing(0);

    // ── Water Depth card ───────────────────────────────────
    QGroupBox *depthCard = makeCard("WATER DEPTH", infoPanel);
    QVBoxLayout *depthLay = new QVBoxLayout(depthCard);
    depthLay->setSpacing(0);

    QHBoxLayout *depthRow = new QHBoxLayout();
    depthValueLabel = makeBigLabel("--");
    depthUnitLabel  = makeSmallLabel("cm");
    depthRow->addWidget(depthValueLabel);
    depthRow->addWidget(depthUnitLabel);
    depthRow->addStretch();
    depthLay->addLayout(depthRow);

    depthBar = makeBar("#26c6da");
    depthLay->addWidget(depthBar);

    sensorStatusLabel = new QLabel("", depthCard);
    sensorStatusLabel->setStyleSheet(
        "color: #ef5350; font-size: 11px; font-weight: bold; background: transparent;");
    sensorStatusLabel->setWordWrap(true);
    sensorStatusLabel->hide();
    depthLay->addWidget(sensorStatusLabel);

    infoLayout->addWidget(depthCard);

    // ── Soil Moisture card ───────────────────────────────────
    QGroupBox *moistureCard = makeCard("SOIL MOISTURE", infoPanel);
    QVBoxLayout *moistureLay = new QVBoxLayout(moistureCard);
    moistureLay->setSpacing(0);

    QHBoxLayout *moistureRow = new QHBoxLayout();
    moistureValueLabel = makeBigLabel("--");
    moistureUnitLabel  = makeSmallLabel("%");
    moistureRow->addWidget(moistureValueLabel);
    moistureRow->addWidget(moistureUnitLabel);
    moistureRow->addStretch();
    moistureLay->addLayout(moistureRow);

    moistureBar = makeBar("#26c6da");
    moistureLay->addWidget(moistureBar);

    moistureStatusLabel = new QLabel("", moistureCard);
    moistureStatusLabel->setStyleSheet(
        "color: #ef5350; font-size: 11px; font-weight: bold; background: transparent;");
    moistureStatusLabel->setWordWrap(true);
    moistureStatusLabel->hide();
    moistureLay->addWidget(moistureStatusLabel);

    infoLayout->addWidget(moistureCard);

    // ── Cumulative Rain card ───────────────────────────────
    QGroupBox *rainCard = makeCard("CUMULATIVE RAIN (2-DAY)", infoPanel);
    QVBoxLayout *rainLay = new QVBoxLayout(rainCard);
    rainLay->setSpacing(0);

    QHBoxLayout *rainRow = new QHBoxLayout();
    rainValueLabel = makeBigLabel("--");
    rainUnitLabel  = makeSmallLabel("mm");
    rainRow->addWidget(rainValueLabel);
    rainRow->addWidget(rainUnitLabel);
    rainRow->addStretch();
    rainLay->addLayout(rainRow);

    rainBar = makeBar("#42a5f5");
    rainLay->addWidget(rainBar);

    infoLayout->addWidget(rainCard);

    // ── System Mode card ───────────────────────────────────
    QGroupBox *modeCard = makeCard("SYSTEM MODE", infoPanel);
    QVBoxLayout *modeLay = new QVBoxLayout(modeCard);
    modeLay->setSpacing(0);

    QHBoxLayout *modeRow = new QHBoxLayout();
    modeIndicator  = makeDot("#66bb6a");
    modeValueLabel = makeBigLabel("MONITORING");
    modeValueLabel->setFont([]{
        QFont f; f.setPixelSize(18); f.setBold(true); return f;
    }());
    modeRow->addWidget(modeIndicator);
    modeRow->addSpacing(0);
    modeRow->addWidget(modeValueLabel);
    modeRow->addStretch();
    modeLay->addLayout(modeRow);

    modeReasonLabel = makeSmallLabel("");
    modeLay->addWidget(modeReasonLabel);

    infoLayout->addWidget(modeCard);

    // ── Valve Status card ──────────────────────────────────
    QGroupBox *valveCard = makeCard("VALVE", infoPanel);
    QVBoxLayout *valveLay = new QVBoxLayout(valveCard);
    valveLay->setSpacing(0);

    QHBoxLayout *valveRow = new QHBoxLayout();
    valveIndicator  = makeDot("#78909c");
    valveValueLabel = makeBigLabel("SHUT");
    valveValueLabel->setFont([]{
        QFont f; f.setPixelSize(18); f.setBold(true); return f;
    }());
    valveRow->addWidget(valveIndicator);
    valveRow->addSpacing(0);
    valveRow->addWidget(valveValueLabel);
    valveRow->addStretch();
    valveLay->addLayout(valveRow);

    // Auto-control checkbox
    autoControlCheckBox = new QCheckBox("Auto Control", valveCard);
    autoControlCheckBox->setChecked(true);
    autoControlCheckBox->setStyleSheet(R"(
        QCheckBox {
            color: #90a4ae;
            font-size: 12px;
            spacing: 6px;
            background: transparent;
        }
        QCheckBox::indicator {
            width: 16px; height: 16px;
            border-radius: 3px;
            border: 1px solid #4a5060;
            background-color: #2b3038;
        }
        QCheckBox::indicator:checked {
            background-color: #0d6efd;
            border-color: #0d6efd;
        }
    )");
    valveLay->addWidget(autoControlCheckBox);
    connect(autoControlCheckBox, &QCheckBox::toggled,
            this, &SmartRainHarvest::onAutoControlToggled);

    // Manual button
    manualButton = new QPushButton("Open Valve", valveCard);
    manualButton->setMinimumHeight(36);
    valveLay->addWidget(manualButton);
    connect(manualButton, &QPushButton::clicked,
            this, &SmartRainHarvest::onManualOpenShut);
    updateValveButton();

    infoLayout->addWidget(valveCard);

    // ── Thresholds card ────────────────────────────────────
    QGroupBox *threshCard = makeCard("THRESHOLDS", infoPanel);
    QGridLayout *threshGrid = new QGridLayout(threshCard);
    threshGrid->setSpacing(0);

    auto addThreshRow = [&](int row, const QString &label, const QString &value) {
        QLabel *l = makeSmallLabel(label);
        QLabel *v = new QLabel(value);
        v->setStyleSheet("color: #b0bec5; font-size: 12px; font-weight: bold; background: transparent;");
        v->setAlignment(Qt::AlignRight);
        threshGrid->addWidget(l, row, 0);
        threshGrid->addWidget(v, row, 1);
        return v;
    };

    threshOverflowLabel       = addThreshRow(0, "Overflow depth",     QString("%1 cm").arg(overflowThreshold));
    threshEmptyLabel       = addThreshRow(1, "Empty depth",     QString("%1 cm").arg(emptyThreshold));
    threshForecastRainLabel   = addThreshRow(2, "Forecast min rain",  QString("%1 mm").arg(forecastThreshold));
    threshmoistureLabel       = addThreshRow(3, "Moisture threshold",    QString("%1 %").arg(moistureThreshold));


    // Separator line
    QFrame *sep = new QFrame(threshCard);
    sep->setFrameShape(QFrame::HLine);
    sep->setStyleSheet("background-color: #2d3139; border: none; max-height: 1px;");
    threshGrid->addWidget(sep, 4, 0, 1, 2);

    addThreshRow(5, "Monitoring interval", QString("%1 s").arg(monitoringInterval));
    addThreshRow(6, "Release interval",    QString("%1 s").arg(releaseInterval));

    infoLayout->addWidget(threshCard);

    infoLayout->addStretch();

    // ════════════════════════════════════════════════════════
    //  Build the charts (right side)
    // ════════════════════════════════════════════════════════

    QSplitter *vSplitter = new QSplitter(Qt::Vertical);
    vSplitter->addWidget(weatherChart->GetChartView());

    QSplitter *hSplitterMiddle = new QSplitter(Qt::Horizontal, vSplitter);
    hSplitterMiddle->addWidget(cumulativeChart->GetChartView());
    hSplitterMiddle->addWidget(depthChart->GetChartView());

    QSplitter *hSplitterBottom = new QSplitter(Qt::Horizontal, vSplitter);
    hSplitterBottom->addWidget(moistureChart->GetChartView());
    hSplitterBottom->addWidget(valveChart->GetChartView());

    //vSplitter->addWidget(valveChart->GetChartView());

    vSplitter->setStretchFactor(0, 1);
    vSplitter->setStretchFactor(1, 1);
    vSplitter->setStretchFactor(2, 1);
    hSplitterMiddle->setStretchFactor(0, 1);
    hSplitterMiddle->setStretchFactor(1, 1);
    hSplitterBottom->setStretchFactor(0, 1);
    hSplitterBottom->setStretchFactor(1, 1);

    // ════════════════════════════════════════════════════════
    //  Main layout: info panel | charts
    // ════════════════════════════════════════════════════════

    QWidget *central = new QWidget(this);
    QHBoxLayout *mainLayout = new QHBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(infoPanel);
    mainLayout->addWidget(vSplitter, 1);

    setCentralWidget(central);

    // Initial state
    updateInfoPanels();
    updateModeIndicator();
}

// ================================================================
//  UI Updates
// ================================================================

void SmartRainHarvest::updateInfoPanels()
{
    // Depth
    depthValueLabel->setText(QString::number(lastDepth, 'f', 1));
    int depthPct = qBound(0, static_cast<int>(lastDepth / barrelDepth * 100), 100);
    depthBar->setValue(depthPct);

    // Moisture
    moistureValueLabel->setText(QString::number(lastMoisture, 'f', 1));
    int moisturePct = qBound(0, static_cast<int>(lastMoisture), 100);
    moistureBar->setValue(moisturePct);

    // Color depth based on danger level
    if (lastDepth > overflowThreshold)
        depthValueLabel->setStyleSheet("color: #ef5350; background: transparent;");
    else
        depthValueLabel->setStyleSheet("color: #26c6da; background: transparent;");

    // Color moisture based on danger level
    if (lastMoisture < moistureThreshold)
        moistureValueLabel->setStyleSheet("color: #ef5350; background: transparent;");
    else
        moistureValueLabel->setStyleSheet("color: #26c6da; background: transparent;");



    // Rain
    rainValueLabel->setText(QString::number(lastCumRain, 'f', 1));
    int rainPct = qBound(0, static_cast<int>(lastCumRain / (forecastThreshold * 2) * 100), 100);
    rainBar->setValue(rainPct);

    if (lastCumRain > forecastThreshold)
        rainValueLabel->setStyleSheet("color: #42a5f5; background: transparent;");
    else
        rainValueLabel->setStyleSheet("color: #eceff1; background: transparent;");
}

void SmartRainHarvest::updateModeIndicator()
{
    if (state == SystemState::Releasing) {
        modeValueLabel->setText("RELEASING");
        modeValueLabel->setStyleSheet("color: #ef5350; background: transparent;");
        modeIndicator->setStyleSheet(
            "background-color: #ef5350; border-radius: 7px; border: none;");

        if (releaseReason == ReleaseReason::Overflow)
            modeReasonLabel->setText("Reason: Overflow protection");
        else if (releaseReason == ReleaseReason::Dry)
            modeReasonLabel->setText("Reason: Dry soil");
        else if (releaseReason == ReleaseReason::Forecast)
            modeReasonLabel->setText("Reason: Rain forcast");
        else
            modeReasonLabel->setText("Reason: Rain forecast and dry soil");
    } else {
        modeValueLabel->setText("MONITORING");
        modeValueLabel->setStyleSheet("color: #66bb6a; background: transparent;");
        modeIndicator->setStyleSheet(
            "background-color: #66bb6a; border-radius: 7px; border: none;");
        modeReasonLabel->setText("");
    }

    // Valve indicator
    if (valveOpen) {
        valveValueLabel->setText("OPEN");
        valveValueLabel->setStyleSheet("color: #ef5350; background: transparent;");
        valveIndicator->setStyleSheet(
            "background-color: #ef5350; border-radius: 7px; border: none;");
    } else {
        valveValueLabel->setText("SHUT");
        valveValueLabel->setStyleSheet("color: #66bb6a; background: transparent;");
        valveIndicator->setStyleSheet(
            "background-color: #66bb6a; border-radius: 7px; border: none;");
    }
}

void SmartRainHarvest::updateValveButton()
{
    if (autoControl) {
        // Auto mode: button is dimmed/disabled-looking
        manualButton->setText(valveOpen ? "Shut Valve" : "Open Valve");
        manualButton->setStyleSheet(R"(
            QPushButton {
                background-color: #37474f;
                color: #78909c;
                border: 1px solid #455a64;
                border-radius: 6px;
                font-weight: bold;
                font-size: 13px;
            }
            QPushButton:hover {
                background-color: #455a64;
            }
        )");
    } else {
        // Manual mode: button is bright and active
        if (valveOpen) {
            manualButton->setText("Shut Valve");
            manualButton->setStyleSheet(R"(
                QPushButton {
                    background-color: #d32f2f;
                    color: white;
                    border: none;
                    border-radius: 6px;
                    font-weight: bold;
                    font-size: 13px;
                }
                QPushButton:hover {
                    background-color: #e53935;
                }
                QPushButton:pressed {
                    background-color: #b71c1c;
                }
            )");
        } else {
            manualButton->setText("Open Valve");
            manualButton->setStyleSheet(R"(
                QPushButton {
                    background-color: #2e7d32;
                    color: white;
                    border: none;
                    border-radius: 6px;
                    font-weight: bold;
                    font-size: 13px;
                }
                QPushButton:hover {
                    background-color: #388e3c;
                }
                QPushButton:pressed {
                    background-color: #1b5e20;
                }
            )");
        }
    }
}

// ================================================================
//  State Transitions
// ================================================================

void SmartRainHarvest::enterMonitoringMode()
{
    state = SystemState::Monitoring;

    releaseTimer->stop();
    monitoringTimer->start(monitoringInterval * 1000);

    depthChart->setAnimated(true);
    valveChart->setAnimated(true);
    cumulativeChart->setAnimated(true);
    moistureChart->setAnimated(true);

    updateModeIndicator();

    //qDebug() << "-> MONITORING mode (interval:"
    //         << monitoringInterval << "s)";
}

void SmartRainHarvest::enterReleaseMode()
{
    // Only auto-release if auto control is on
    if (!autoControl) {
        //qDebug() << "Auto control OFF — skipping release";
        return;
    }

    state = SystemState::Releasing;

    depthChart->setAnimated(false);
    valveChart->setAnimated(false);
    cumulativeChart->setAnimated(false);
    moistureChart->setAnimated(false);

    openValve();
    recordValveState();
    dbWriter.sendValveState(true);

    monitoringTimer->stop();
    releaseTimer->start(releaseInterval * 1000);

    updateModeIndicator();
    updateValveButton();
}

// ================================================================
//  MONITORING tick
// ================================================================

void SmartRainHarvest::onMonitoringTick()
{
    //qDebug() << "-- Monitoring tick --";


    if (autoControl)
    {
        if (checkIfShouldRelease()) // Enter release mode if conditions are met.
        {
            enterReleaseMode();
            return;
        }
    }


    recordValveState();
}

// ================================================================
//  RELEASE tick
// ================================================================

void SmartRainHarvest::onReleaseTick()
{
    //qDebug() << "-- Release tick --";



    if (!checkIfShouldRelease()) // Enter monitoring mode if conditions are met.
    {

        shutValve();
        recordValveState();
        dbWriter.sendValveState(false);

        enterMonitoringMode();
        updateValveButton();
    }
    else
    {

        recordValveState();
        dbWriter.sendValveState(true);
    }
}

// ================================================================
//  Auto Control Toggle
// ================================================================

void SmartRainHarvest::onAutoControlToggled(bool checked)
{
    autoControl = checked;

    if (checked) {
        //qDebug() << "Auto control ENABLED";

        // If valve was manually opened, shut it and reset
        if (valveOpen && state != SystemState::Releasing) {
            shutValve();
            recordValveState();
            dbWriter.sendValveState(false);
        }

        // Make sure we're in monitoring mode
        if (state != SystemState::Monitoring)
            enterMonitoringMode();

        updateModeIndicator();
    } else {
        //qDebug() << "Auto control DISABLED (manual mode)";

        // If currently releasing, stop
        if (state == SystemState::Releasing) {
            shutValve();
            recordValveState();
            dbWriter.sendValveState(false);
            enterMonitoringMode();
        }
    }

    updateValveButton();
}

// ================================================================
//  Manual Override
// ================================================================

void SmartRainHarvest::onManualOpenShut()
{
    // Uncheck auto control
    autoControlCheckBox->setChecked(false);
    autoControl = false;

    if (valveOpen) {
        shutValve();

        if (state == SystemState::Releasing)
            enterMonitoringMode();
    } else {
        openValve();
    }

    recordValveState();
    dbWriter.sendValveState(valveOpen);
    updateModeIndicator();
    updateValveButton();
}

// ================================================================
//  Depth Measurement
// ================================================================

double SmartRainHarvest::measureDepth()
{
    double raw = distanceSensor.getDistance();

    // Sensor returned error (-1)
    if (raw < 0) {
        sensorFailCount++;
        qWarning() << "Sensor read failed (" << sensorFailCount
                   << "/" << MAX_SENSOR_FAILS << ")";

        if (sensorFailCount >= MAX_SENSOR_FAILS) {
            sensorStatusLabel->setText(
                "⚠ Sensor not connected or malfunctioning");
            sensorStatusLabel->show();
            depthValueLabel->setText("--");
            depthValueLabel->setStyleSheet(
                "color: #78909c; background: transparent;");
            depthBar->setValue(0);
        }

        return lastDepth;  // Return last known good value
    }

    // Sensor OK — reset fail counter
    if (sensorFailCount >= MAX_SENSOR_FAILS) {
        sensorStatusLabel->hide();  // Clear error when sensor recovers
    }
    sensorFailCount = 0;

    double depth = barrelDepth - raw;

    if (depth > barrelDepth) depth = barrelDepth;
    if (depth < 0)           depth = 0;

    //qDebug() << "Depth:" << depth << "cm (raw sensor:" << raw << "cm)";
    return depth;
}

double SmartRainHarvest::measureMoisture()
{
    double raw = moistureSensor.getMoisture();

    double moisture = raw;

    return moisture;
}



// ================================================================
//  Valve Control
// ================================================================

void SmartRainHarvest::openValve()
{
#ifdef RasPi
   //digitalWrite(VALVE_PIN, HIGH);
   digitalWrite(VALVE_CLOSE_PIN, LOW);
   delay(50);

   digitalWrite(VALVE_OPEN_PIN, HIGH);
   delay(VALVE_PULSE_MS);
   digitalWrite(VALVE_OPEN_PIN, LOW);
#endif
    //qDebug() << "VALVE OPENED";

    valveOpen = true;
}

void SmartRainHarvest::shutValve()
{
#ifdef RasPi
    //digitalWrite(VALVE_PIN, LOW);
    digitalWrite(VALVE_OPEN_PIN, LOW);
    delay(50);
    digitalWrite(VALVE_CLOSE_PIN, LOW);
    digitalWrite(VALVE_CLOSE_PIN, HIGH);
    delay(VALVE_PULSE_MS);
    //digitalWrite(VALVE_CLOSE_PIN, LOW);
#endif
    //qDebug() << "VALVE SHUT";

    valveOpen = false;
}

// ================================================================
//  Data Recording
// ================================================================

void SmartRainHarvest::recordDepth(double depth)
{
    if (depthHistory.count() > MAX_HISTORY)
        depthHistory.removeFirst();
    depthHistory.append({QDateTime::currentDateTime(), depth});
    depthChart->plotWeatherData(depthHistory, "Water Depth (cm)");
}

void SmartRainHarvest::recordValveState()
{
    if (valveHistory.count() > MAX_HISTORY)
        valveHistory.removeFirst();
    valveHistory.append({QDateTime::currentDateTime(),
                         static_cast<double>(valveOpen)});
    valveChart->plotWeatherData(valveHistory, "Valve State (on/off)");
}

void SmartRainHarvest::recordMoisture(double moisture)
{
    if (moistureHistory.count() > MAX_HISTORY)
        moistureHistory.removeFirst();
    moistureHistory.append({QDateTime::currentDateTime(), moisture});
    moistureChart->plotWeatherData(moistureHistory, "Moisture Level (%)");
}



bool SmartRainHarvest::checkIfShouldRelease()
{
    // Check states.
    lastDepth = measureDepth();
    recordDepth(lastDepth);
    dbWriter.sendDepthReading(lastDepth);

    lastMoisture = measureMoisture();
    recordMoisture(lastMoisture);
    dbWriter.sendMoistureReading(lastMoisture);


    // 1. Fetch weather
    QVector<WeatherData> rainAmount = fetcher.getWeatherPrediction(
        gridX, gridY, datatype::PrecipitationAmount);
    QVector<WeatherData> rainProb = fetcher.getWeatherPrediction(
        gridX, gridY, datatype::ProbabilityofPrecipitation);
    QVector<WeatherData> temp = fetcher.getWeatherPrediction(
        gridX, gridY, datatype::Temperature);

    QMap<QString, QVector<WeatherData>> forecastMap;
    forecastMap["Precipitation [mm]"]            = rainAmount;
    forecastMap["Precipitation probability (%)"] = rainProb;
    forecastMap["Temperature (<sup>o</sup>C)"]   = temp;
    weatherChart->plotWeatherDataMap(forecastMap);
    weatherChart->GetChartView()->setRenderHint(QPainter::Antialiasing);

    dbWriter.sendWeatherData("precip_amount", "mm",  rainAmount);
    dbWriter.sendWeatherData("precip_prob",   "%",   rainProb);
    dbWriter.sendWeatherData("temperature",   "C",   temp);

    // 2. Cumulative rain
    lastCumRain = calculateCumulativeValue(rainAmount, 2);

    //qDebug() << "Cumulative rain (2-day):" << lastCumRain << "mm from" << rainAmount.size() << "data points";



    // Keep cumulative rain chart updating with last known value
    if (cumulativeRainHistory.count() > MAX_HISTORY)
        cumulativeRainHistory.removeFirst();
    cumulativeRainHistory.append({QDateTime::currentDateTime(), lastCumRain});
    //cumulativeChart->setAnimated(false);
    cumulativeChart->plotWeatherData(cumulativeRainHistory,
                                     "Cumulative rain forecast [mm]");



    // Safety: if sensor fails repeatedly during release, shut valve
    if (sensorFailCount >= MAX_SENSOR_FAILS && state == SystemState::Releasing) {
        qWarning() << "Sensor failed during release — shutting valve for safety";
        shutValve();
        recordValveState();
        dbWriter.sendValveState(false);
        enterMonitoringMode();
        updateValveButton();
        return false;
    }




    releaseReason = ReleaseReason::None;

    if (lastMoisture < moistureThreshold && lastCumRain < forecastThreshold)
        releaseReason = ReleaseReason::DryAndForecast;
    else if (lastMoisture < moistureThreshold)
        releaseReason = ReleaseReason::Dry;
    else if (lastCumRain < forecastThreshold)
        releaseReason = ReleaseReason::Forecast;
    if (lastDepth > overflowThreshold)
        releaseReason = ReleaseReason::Overflow;


    updateInfoPanels();
    SmartRainHarvest::updateModeIndicator();


    // Decide state.
    return (!(lastDepth < emptyThreshold || lastMoisture > moistureThreshold || lastCumRain > forecastThreshold )
            || lastDepth > overflowThreshold); // Release

}
