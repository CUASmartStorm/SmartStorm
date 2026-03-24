/////////////////////////////////////////////////////////////
// DISTANCESENSOR.H - Distance Sensor Class Header
/////////////////////////////////////////////////////////////

#ifndef MOISTURESENSOR_H
#define MOISTURESENSOR_H

#include <QObject>
#include <QDebug>
#include <QThread>
#ifdef RasPi
#include <wiringPi.h>
#include <wiringPiSPI.h>
#endif

// Class for managing HC-SR04 ultrasonic distance sensor
class MoistureSensor : public QObject {
    Q_OBJECT

public:
    MoistureSensor();
    ~MoistureSensor();

    bool initialize();      // Setup GPIO pins
    void cleanup();         // Cleanup resources

    int readChannel(int channel);
    double rawToMoisturePercent(int raw);
    double getMoisture();   // Get distance measurement in cm

private:
    //const int TRIG_PIN = 4;  // BCM pin 4 - Trigger pin
    //const int ECHO_PIN = 17; // BCM pin 17 - Echo pin
};

#endif // MOISTURESENSOR_H

