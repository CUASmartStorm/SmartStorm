/////////////////////////////////////////////////////////////
// DISTANCESENSOR.CPP - Soil Moisture Sensor Control
/////////////////////////////////////////////////////////////

#include "MoistureSensor.h"
#include <cmath>


MoistureSensor::MoistureSensor() {
}

MoistureSensor::~MoistureSensor() {
    cleanup();
}

bool MoistureSensor::initialize() {
#ifdef RasPi
    wiringPiSPISetup(0, 1350000); // SPI channel 0, 1.35 MHz
    //wiringPiSetupGpio();
    //pinMode(TRIG_PIN, OUTPUT);
    //pinMode(ECHO_PIN, INPUT);
    //digitalWrite(TRIG_PIN, LOW);
    //QThread::msleep(50);  // Let sensor settle
#endif
    return true;
}

void MoistureSensor::cleanup() {
}

int MoistureSensor::readChannel(int channel) {

    unsigned char buf[3];

    buf[0] = 0x01;
    buf[1] = (0x08 | channel) << 4;
    buf[2] = 0x00;

    wiringPiSPIDataRW(0, buf,3);

    return ((buf[1] & 0x03) << 8) | buf[2];
}

double MoistureSensor::rawToMoisturePercent(int raw) {

    const int DRY = 580;
    const int WET = 280;
    double pct = 100.0 * (DRY - raw) / (DRY - WET);

    return qBound(0.0, pct, 100.0);
}

double MoistureSensor::getMoisture() {
#ifdef RasPi

    //for (int ch = 0; ch < 8; ch++) {
    //    int raw = readChannel(ch);
    //    double moisture = rawToMoisturePercent(raw);
    //}

    int raw = readChannel(7);
    double moisture = rawToMoisturePercent(raw);

    qDebug() << "Moisture :"
             << moisture;


    return std::round(moisture);
#endif

    // Non-RasPi: simulate mid-barrel reading for testing
    return 0.0;
}
