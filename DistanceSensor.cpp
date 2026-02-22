/////////////////////////////////////////////////////////////
// DISTANCESENSOR.CPP - Ultrasonic Distance Sensor Control
/////////////////////////////////////////////////////////////

#include "DistanceSensor.h"
#include <cmath>

DistanceSensor::DistanceSensor() {
}

DistanceSensor::~DistanceSensor() {
    cleanup();
}

bool DistanceSensor::initialize() {
#ifdef RasPi
    wiringPiSetupGpio();
    pinMode(TRIG_PIN, OUTPUT);
    pinMode(ECHO_PIN, INPUT);
    digitalWrite(TRIG_PIN, LOW);
    QThread::msleep(50);  // Let sensor settle
#endif
    return true;
}

void DistanceSensor::cleanup() {
}

double DistanceSensor::getDistance() {
#ifdef RasPi
    // ── Send trigger pulse ─────────────────────────────────
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);

    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    // ── Wait for ECHO to go HIGH (start of pulse) ─────────
    // Timeout: 30ms — if echo never goes HIGH, sensor is
    // disconnected or on the wrong pin
    long timeout = micros() + 30000;
    while (digitalRead(ECHO_PIN) == LOW) {
        if (micros() >= timeout) {
            qWarning() << "DistanceSensor: ECHO never went HIGH"
                       << "— sensor may be disconnected";
            return -1;
        }
    }
    long pulseStart = micros();

    // ── Wait for ECHO to go LOW (end of pulse) ────────────
    // Timeout: 30ms — covers max range (~5m)
    timeout = micros() + 30000;
    while (digitalRead(ECHO_PIN) == HIGH) {
        if (micros() >= timeout) {
            qWarning() << "DistanceSensor: ECHO stuck HIGH"
                       << "— sensor may be malfunctioning";
            return -1;
        }
    }
    long pulseEnd = micros();

    // ── Calculate distance ─────────────────────────────────
    double pulseDuration = pulseEnd - pulseStart;
    double distance = pulseDuration * 0.01715;  // cm

    // Sanity check: HC-SR04 range is 2–400 cm
    if (distance < 0 || distance > 400) {
        qWarning() << "DistanceSensor: reading out of range:"
                   << distance << "cm";
        return -1;
    }

    return std::round(distance * 100.0) / 100.0;
#endif

    // Non-RasPi: simulate mid-barrel reading for testing
    return 50;
}
