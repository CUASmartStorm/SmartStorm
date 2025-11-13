/////////////////////////////////////////////////////////////
// DISTANCESENSOR.H - Distance Sensor Class Header
/////////////////////////////////////////////////////////////

#ifndef DISTANCESENSOR_H
#define DISTANCESENSOR_H

#include <QObject>
#include <QDebug>
#include <QThread>
#ifdef RasPi
#include <wiringPi.h>
#endif

// Class for managing HC-SR04 ultrasonic distance sensor
class DistanceSensor : public QObject {
    Q_OBJECT

public:
    DistanceSensor();
    ~DistanceSensor();

    bool initialize();      // Setup GPIO pins
    void cleanup();         // Cleanup resources
    double getDistance();   // Get distance measurement in cm

private:
    const int TRIG_PIN = 4;  // BCM pin 4 - Trigger pin
    const int ECHO_PIN = 17; // BCM pin 17 - Echo pin
};

#endif // DISTANCESENSOR_H

