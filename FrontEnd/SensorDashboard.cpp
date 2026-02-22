#include "SensorDashboard.h"
#include <QLinearGradient>
#include <QGraphicsDropShadowEffect>
#include <QFont>

SensorDashboard::SensorDashboard(QWidget *parent)
    : QMainWindow(parent)
{
    apiUrl = "http://54.213.147.59:5000";
    networkManager = new QNetworkAccessManager(this);
    pendingRequests = 0;

    // Fallback sensor list
    sensorIds = {
        "precip_amount",
        "precip_prob",
        "temperature",
        "water_depth",
        "valve_state"
    };

    // Auto-refresh timer (60 s)
    refreshTimer = new QTimer(this);
    refreshTimer->setInterval(60000);
    connect(refreshTimer, &QTimer::timeout,
            this, &SensorDashboard::onAutoRefreshTimeout);

    // Countdown display timer
    countdownTimer = new QTimer(this);
    countdownTimer->setInterval(1000);
    connect(countdownTimer, &QTimer::timeout, this, [this]() {
        countdownSeconds--;
        if (countdownSeconds >= 0)
            countdownLabel->setText(QString("  %1s").arg(countdownSeconds));
    });
    countdownSeconds = 60;

    setupUI();
    fetchSensorList();
}

SensorDashboard::~SensorDashboard()
{
}

// ================================================================
//  UI
// ================================================================

void SensorDashboard::setupUI()
{
    setWindowTitle("SmartRainHarvest — Sensor Dashboard");
    resize(1200, 850);

    // ── Global stylesheet (modern, flat) ───────────────────────
    setStyleSheet(R"(
        QMainWindow {
            background-color: #1a1d23;
        }
        QGroupBox {
            font-weight: bold;
            font-size: 13px;
            color: #b0bec5;
            border: 1px solid #2d3139;
            border-radius: 8px;
            margin-top: 10px;
            padding: 14px 10px 8px 10px;
            background-color: #21252b;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 16px;
            padding: 0 8px;
        }
        QPushButton {
            background-color: #0d6efd;
            color: white;
            border: none;
            border-radius: 6px;
            padding: 7px 22px;
            font-weight: bold;
            font-size: 13px;
        }
        QPushButton:hover {
            background-color: #3d8bfd;
        }
        QPushButton:pressed {
            background-color: #0a58ca;
        }
        QDateTimeEdit {
            background-color: #2b3038;
            color: #e0e0e0;
            border: 1px solid #3a3f47;
            border-radius: 6px;
            padding: 5px 10px;
            font-size: 13px;
        }
        QDateTimeEdit::drop-down {
            border: none;
            width: 20px;
        }
        QLabel {
            color: #90a4ae;
            font-size: 13px;
        }
        QCheckBox {
            color: #90a4ae;
            font-size: 13px;
            spacing: 6px;
        }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border-radius: 3px;
            border: 1px solid #4a5060;
            background-color: #2b3038;
        }
        QCheckBox::indicator:checked {
            background-color: #0d6efd;
            border-color: #0d6efd;
        }
        QStatusBar {
            background-color: #181b20;
            color: #607d8b;
            font-size: 12px;
            border-top: 1px solid #2d3139;
        }
        QScrollArea {
            background-color: transparent;
            border: none;
        }
    )");

    centralWidget = new QWidget(this);
    mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(10);

    // === Control Panel ===
    controlGroup = new QGroupBox("Query Controls", this);
    QHBoxLayout *controlRow = new QHBoxLayout(controlGroup);
    controlRow->setSpacing(10);

    startLabel = new QLabel("From:", this);
    startDateTimeEdit = new QDateTimeEdit(this);
    startDateTimeEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    startDateTimeEdit->setCalendarPopup(true);
    startDateTimeEdit->setDateTime(QDateTime::currentDateTime().addDays(-7));

    endLabel = new QLabel("To:", this);
    endDateTimeEdit = new QDateTimeEdit(this);
    endDateTimeEdit->setDisplayFormat("yyyy-MM-dd HH:mm");
    endDateTimeEdit->setCalendarPopup(true);
    endDateTimeEdit->setDateTime(QDateTime::currentDateTime().addDays(7));

    fetchButton = new QPushButton("Fetch Data", this);

    autoRefreshCheckBox = new QCheckBox("Auto-refresh (60s)", this);

    countdownLabel = new QLabel("", this);
    countdownLabel->setStyleSheet("color: #546e7a; font-style: italic;");

    controlRow->addWidget(startLabel);
    controlRow->addWidget(startDateTimeEdit);
    controlRow->addWidget(endLabel);
    controlRow->addWidget(endDateTimeEdit);
    controlRow->addWidget(fetchButton);
    controlRow->addSpacing(24);
    controlRow->addWidget(autoRefreshCheckBox);
    controlRow->addWidget(countdownLabel);
    controlRow->addStretch();

    // === Charts Area ===
    chartsContainer = new QWidget();
    chartsContainer->setStyleSheet("background-color: transparent;");
    chartsLayout = new QVBoxLayout(chartsContainer);
    chartsLayout->setContentsMargins(0, 0, 0, 0);

#ifdef SCROLLABLE_CHARTS
    chartsLayout->setSpacing(10);
    scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setWidget(chartsContainer);

    mainLayout->addWidget(controlGroup);
    mainLayout->addWidget(scrollArea, 1);
#else
    chartsLayout->setSpacing(4);
    mainLayout->addWidget(controlGroup);
    mainLayout->addWidget(chartsContainer, 1);
#endif

    setCentralWidget(centralWidget);
    statusBar()->showMessage("Ready — default range: ±7 days");

    // === Signals ===
    connect(fetchButton, &QPushButton::clicked,
            this, &SensorDashboard::onFetchClicked);
    connect(autoRefreshCheckBox, &QCheckBox::toggled,
            this, &SensorDashboard::onAutoRefreshToggled);
}

// ================================================================
//  Slots
// ================================================================

void SensorDashboard::onFetchClicked()
{
    fetchAllSensors();
}

void SensorDashboard::onAutoRefreshToggled(bool checked)
{
    if (checked) {
        countdownSeconds = 60;
        countdownLabel->setText(QString("  %1s").arg(countdownSeconds));
        refreshTimer->start();
        countdownTimer->start();
    } else {
        refreshTimer->stop();
        countdownTimer->stop();
        countdownLabel->setText("");
    }
}

void SensorDashboard::onAutoRefreshTimeout()
{
    endDateTimeEdit->setDateTime(QDateTime::currentDateTime().addDays(7));
    fetchAllSensors();
    countdownSeconds = 60;
}

// ================================================================
//  Network — sensor list
// ================================================================

void SensorDashboard::fetchSensorList()
{
    QUrl url(apiUrl + "/sensors");
    QNetworkRequest request(url);

    qDebug() << "Fetching sensor list from" << url.toString();

    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onSensorListReceived(reply);
    });
}

void SensorDashboard::onSensorListReceived(QNetworkReply *reply)
{
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray data = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(data);

        if (doc.isArray()) {
            QJsonArray arr = doc.array();
            QStringList newIds;
            for (const QJsonValue &v : arr)
                newIds.append(v.toString());
            if (!newIds.isEmpty()) {
                sensorIds = newIds;
                qDebug() << "Loaded sensors from API:" << sensorIds;
            }
        }
    } else {
        qDebug() << "Could not fetch sensor list, using defaults:"
                 << reply->errorString();
    }

    reply->deleteLater();
    fetchAllSensors();
}

// ================================================================
//  Network — sensor data
// ================================================================

void SensorDashboard::fetchAllSensors()
{
    pendingRequests = sensorIds.size();
    setStatus(QString("Fetching data for %1 sensors...").arg(pendingRequests));

    for (const QString &id : sensorIds)
        fetchSensorData(id);
}

void SensorDashboard::fetchSensorData(const QString &sensorId)
{
    QUrl url(apiUrl + "/sensor/" + sensorId);
    QUrlQuery query;

    QDateTime startDt = startDateTimeEdit->dateTime();
    QDateTime endDt   = endDateTimeEdit->dateTime();

    query.addQueryItem("start", startDt.toUTC().toString(Qt::ISODate));
    query.addQueryItem("end",   endDt.toUTC().toString(Qt::ISODate));
    url.setQuery(query);

    QNetworkRequest request(url);

    QNetworkReply *reply = networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, sensorId, reply]() {
        onDataReceived(sensorId, reply);
    });
}

void SensorDashboard::onDataReceived(const QString &sensorId,
                                     QNetworkReply *reply)
{
    if (!reply) return;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray responseData = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(responseData);

        QJsonArray dataArray;
        if (doc.isArray()) {
            dataArray = doc.array();
        } else if (doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("readings"))
                dataArray = obj["readings"].toArray();
        }

        updateChart(sensorId, dataArray);
    } else {
        qDebug() << "Error fetching" << sensorId << ":" << reply->errorString();
        QJsonArray empty;
        updateChart(sensorId, empty);
    }

    reply->deleteLater();

    pendingRequests--;
    if (pendingRequests <= 0) {
        setStatus(QString("All sensors loaded — %1")
                      .arg(QDateTime::currentDateTime().toString("hh:mm:ss")));
    }
}

// ================================================================
//  Chart management
// ================================================================

SensorChart &SensorDashboard::getOrCreateChart(const QString &sensorId)
{
    if (!sensorCharts.contains(sensorId)) {
        SensorChart sc;

        // ── Chart ──────────────────────────────────────────────
        sc.chart = new QChart();
        sc.chart->setAnimationOptions(QChart::SeriesAnimations);
        sc.chart->setBackgroundBrush(QBrush(QColor("#21252b")));
        sc.chart->setBackgroundRoundness(10);
        sc.chart->setMargins(QMargins(12, 8, 12, 8));
        sc.chart->legend()->hide();

        // Title
        QFont titleFont;
        titleFont.setPixelSize(14);
        titleFont.setBold(true);
        sc.chart->setTitleFont(titleFont);
        sc.chart->setTitleBrush(QBrush(QColor("#cfd8dc")));
        sc.chart->setTitle(friendlyName(sensorId));

        // ── Line series ────────────────────────────────────────
        sc.series = new QLineSeries();
        sc.series->setName(sensorId);
        QPen linePen(seriesColor(sensorId));
        linePen.setWidth(2);
        sc.series->setPen(linePen);

        // ── Area fill under curve ──────────────────────────────
        QLineSeries *lower = new QLineSeries();   // stays at 0
        sc.area = new QAreaSeries(sc.series, lower);
        sc.area->setName(sensorId);

        QColor fill = areaColor(sensorId);
        sc.area->setBrush(QBrush(fill));
        sc.area->setPen(linePen);           // top edge = line pen
        QPen noPen(Qt::NoPen);
        sc.area->setBorderColor(Qt::transparent);

        sc.chart->addSeries(sc.area);

        // ── X axis (time) ──────────────────────────────────────
        sc.axisX = new QDateTimeAxis();
        sc.axisX->setFormat("MM/dd HH:mm");
        sc.axisX->setTitleText("Time");
        sc.axisX->setLabelsAngle(-30);
        sc.axisX->setGridLineVisible(true);
        sc.axisX->setGridLineColor(QColor("#2d3139"));
        sc.axisX->setLinePenColor(QColor("#3a3f47"));
        sc.axisX->setLabelsColor(QColor("#78909c"));
        QFont axisFont;
        axisFont.setPixelSize(11);
        sc.axisX->setLabelsFont(axisFont);
        QFont axisTitleFont;
        axisTitleFont.setPixelSize(12);
        sc.axisX->setTitleFont(axisTitleFont);
        sc.axisX->setTitleBrush(QBrush(QColor("#607d8b")));
        sc.chart->addAxis(sc.axisX, Qt::AlignBottom);
        sc.area->attachAxis(sc.axisX);

        // ── Y axis (value) ─────────────────────────────────────
        sc.axisY = new QValueAxis();
        sc.axisY->setTitleText(unitLabel(sensorId));
        sc.axisY->setGridLineVisible(true);
        sc.axisY->setGridLineColor(QColor("#2d3139"));
        sc.axisY->setLinePenColor(QColor("#3a3f47"));
        sc.axisY->setLabelsColor(QColor("#78909c"));
        sc.axisY->setLabelsFont(axisFont);
        sc.axisY->setTitleFont(axisTitleFont);
        sc.axisY->setTitleBrush(QBrush(QColor("#607d8b")));
        sc.chart->addAxis(sc.axisY, Qt::AlignLeft);
        sc.area->attachAxis(sc.axisY);

        // ── Chart view ─────────────────────────────────────────
        sc.chartView = new QChartView(sc.chart);
        sc.chartView->setRenderHint(QPainter::Antialiasing);
        sc.chartView->setStyleSheet(
            "background-color: #21252b; border-radius: 10px;");

#ifdef SCROLLABLE_CHARTS
        sc.chartView->setMinimumHeight(280);
        sc.chartView->setMaximumHeight(360);
#else
        sc.chartView->setSizePolicy(
            QSizePolicy::Expanding, QSizePolicy::Expanding);
#endif

        // Insert in order
        int insertPos = chartsLayout->count();
        for (int i = 0; i < sensorIds.size(); ++i) {
            if (sensorIds[i] == sensorId) {
                insertPos = i;
                break;
            }
        }
        if (insertPos > chartsLayout->count())
            insertPos = chartsLayout->count();
        chartsLayout->insertWidget(insertPos, sc.chartView, 1);

        sensorCharts[sensorId] = sc;
    }

    return sensorCharts[sensorId];
}

void SensorDashboard::updateChart(const QString &sensorId,
                                  const QJsonArray &dataArray)
{
    SensorChart &sc = getOrCreateChart(sensorId);
    sc.series->clear();

    // Also clear the lower bound series of the area
    if (sc.area && sc.area->lowerSeries())
        sc.area->lowerSeries()->clear();

    if (dataArray.isEmpty()) {
        sc.chart->setTitle(friendlyName(sensorId) + "  (no data)");
        return;
    }

    QString unit;
    double minVal =  std::numeric_limits<double>::max();
    double maxVal =  std::numeric_limits<double>::lowest();
    QDateTime minTime, maxTime;

    for (const QJsonValue &val : dataArray) {
        if (!val.isObject()) continue;
        QJsonObject reading = val.toObject();

        QString tsStr = reading["timestamp"].toString();
        QDateTime ts = QDateTime::fromString(tsStr, Qt::ISODate);
        if (!ts.isValid()) continue;

        double v = 0.0;
        QJsonValue vField = reading["value"];
        if (vField.isString())
            v = vField.toString().toDouble();
        else if (vField.isDouble())
            v = vField.toDouble();
        else
            continue;

        if (unit.isEmpty() && reading.contains("unit"))
            unit = reading["unit"].toString();

        sc.series->append(ts.toMSecsSinceEpoch(), v);

        if (v < minVal) minVal = v;
        if (v > maxVal) maxVal = v;
        if (!minTime.isValid() || ts < minTime) minTime = ts;
        if (!maxTime.isValid() || ts > maxTime) maxTime = ts;
    }

    // Fill the lower bound series so the area renders properly
    if (sc.area && sc.area->lowerSeries() && sc.series->count() > 0) {
        QLineSeries *lower = qobject_cast<QLineSeries *>(sc.area->lowerSeries());
        if (lower) {
            lower->clear();
            double floor = floorAtZero(sensorId) ? 0.0 : minVal;
            for (const QPointF &pt : sc.series->points())
                lower->append(pt.x(), floor);
        }
    }

    if (sc.series->count() == 0) {
        sc.chart->setTitle(friendlyName(sensorId) + "  (no valid data)");
        return;
    }

    // Title
    sc.chart->setTitle(QString("%1  —  %2 readings")
                           .arg(friendlyName(sensorId))
                           .arg(sc.series->count()));

    // Y axis
    if (!unit.isEmpty())
        sc.axisY->setTitleText(unit);

    double yMin = minVal;
    double yMax = maxVal;

    // Floor at zero for precipitation sensors
    if (floorAtZero(sensorId))
        yMin = 0.0;

    double range = yMax - yMin;
    double pad   = range * 0.1;
    if (range == 0) pad = (yMax != 0) ? qAbs(yMax) * 0.1 : 1.0;

    // Don't go below zero for floored sensors
    double lower = floorAtZero(sensorId) ? 0.0 : (yMin - pad);
    sc.axisY->setRange(lower, yMax + pad);

    // X axis
    if (minTime.isValid() && maxTime.isValid())
        sc.axisX->setRange(minTime, maxTime);
}

// ================================================================
//  Helpers
// ================================================================

void SensorDashboard::setStatus(const QString &message)
{
    statusBar()->showMessage(message);
}

QString SensorDashboard::friendlyName(const QString &sensorId)
{
    if (sensorId == "precip_amount")  return "Precipitation Amount";
    if (sensorId == "precip_prob")    return "Precipitation Probability";
    if (sensorId == "temperature")    return "Temperature";
    if (sensorId == "water_depth")    return "Water Depth";
    if (sensorId == "valve_state")    return "Valve State";
    QString name = sensorId;
    name.replace('_', ' ');
    if (!name.isEmpty()) name[0] = name[0].toUpper();
    return name;
}

QString SensorDashboard::unitLabel(const QString &sensorId)
{
    if (sensorId == "precip_amount")  return "mm";
    if (sensorId == "precip_prob")    return "%";
    if (sensorId == "temperature")    return "°C";
    if (sensorId == "water_depth")    return "cm";
    if (sensorId == "valve_state")    return "0 / 1";
    return "Value";
}

QColor SensorDashboard::seriesColor(const QString &sensorId)
{
    if (sensorId == "precip_amount")  return QColor("#42a5f5"); // blue
    if (sensorId == "precip_prob")    return QColor("#ab47bc"); // purple
    if (sensorId == "temperature")    return QColor("#ef5350"); // red
    if (sensorId == "water_depth")    return QColor("#26c6da"); // cyan
    if (sensorId == "valve_state")    return QColor("#66bb6a"); // green
    return QColor("#78909c");
}

QColor SensorDashboard::areaColor(const QString &sensorId)
{
    // Translucent fill under the curve
    if (sensorId == "precip_amount")  return QColor(66,165,245, 45);
    if (sensorId == "precip_prob")    return QColor(171,71,188, 40);
    if (sensorId == "temperature")    return QColor(239,83,80,  40);
    if (sensorId == "water_depth")    return QColor(38,198,218,  40);
    if (sensorId == "valve_state")    return QColor(102,187,106, 40);
    return QColor(120,144,156, 30);
}

bool SensorDashboard::floorAtZero(const QString &sensorId)
{
    // These sensors should always start Y axis at 0
    return (sensorId == "precip_amount" ||
            sensorId == "precip_prob"   ||
            sensorId == "valve_state");
}
