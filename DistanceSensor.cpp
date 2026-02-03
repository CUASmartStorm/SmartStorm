
/////////////////////////////////////////////////////////////
// DISTANCESENSOR.CPP - Ultrasonic Distance Sensor Control
/////////////////////////////////////////////////////////////

#include "DistanceSensor.h"
#include <cmath>

// Constructor
DistanceSensor::DistanceSensor() {
}

// Destructor - cleanup resources
DistanceSensor::~DistanceSensor() {
    cleanup();
}

// Initialize the distance sensor GPIO pins
bool DistanceSensor::initialize() {
#ifdef RasPi
    wiringPiSetupGpio(); // Use BCM pin numbering
    pinMode(TRIG_PIN, OUTPUT);   // Trigger pin
    pinMode(ECHO_PIN, INPUT);   // Echo pin
#endif
    return true;
}

// Cleanup function (placeholder for future resource management)
void DistanceSensor::cleanup() {
    // Cleanup resources if needed
}

double DistanceSensor::getDistance() {
    // Clear the TRIG_PIN
#ifdef RasPi
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2); // Wait for 2 microseconds

    // Send a trigger pulse
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10); // Originally 10 micros.
    digitalWrite(TRIG_PIN, LOW);

    // Wait for ECHO_PIN to go HIGH
    long timeout = micros() + 300000;

    while (digitalRead(ECHO_PIN) == LOW && micros() < timeout); // Need to activate later
    if (micros() >= timeout) return -1;

    long pulseStart = micros();

    // Wait for ECHO_PIN to go LOW
    timeout = micros() + 300000;
    while (digitalRead(ECHO_PIN) == HIGH && micros() < timeout);
    if (micros() >= timeout) return -1;

    long pulseEnd = micros();

    // Calculate the distance
    double pulseDuration = pulseEnd - pulseStart;
    double distance = pulseDuration * 0.01715; // Distance in cm


    return std::round(distance * 100.0) / 100.0; // Round to 2 decimal places
#endif

    return 0;
}
