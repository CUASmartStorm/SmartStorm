
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
    pinMode(4, OUTPUT);   // Trigger pin
    pinMode(17, INPUT);   // Echo pin
#endif
    return true;
}

// Cleanup function (placeholder for future resource management)
void DistanceSensor::cleanup() {
    // Cleanup resources if needed
}

// Measure distance using ultrasonic sensor
double DistanceSensor::getDistance() {
#ifdef RasPi
    // Clear the TRIG_PIN
    digitalWrite(4, LOW);
    QThread::sleep(2); // Wait for 2 seconds

    // Send a 10 microsecond trigger pulse
    digitalWrite(4, HIGH);
    delayMicroseconds(10);
    digitalWrite(4, LOW);

    // Wait for ECHO_PIN to go HIGH (start of pulse)
    while (digitalRead(17) == LOW);
    long pulseStart = micros();

    // Wait for ECHO_PIN to go LOW (end of pulse)
    while (digitalRead(17) == HIGH);
    long pulseEnd = micros();

    // Calculate the distance based on pulse duration
    // Speed of sound = 343 m/s = 0.0343 cm/us
    // Distance = (duration / 2) * speed of sound
    double pulseDuration = pulseEnd - pulseStart;
    double distance = pulseDuration * 0.01715; // Distance in cm

    return std::round(distance * 100.0) / 100.0; // Round to 2 decimal places
#endif

    // Return random value for testing when not on Raspberry Pi
    return rand() % 100;
}


