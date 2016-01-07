#ifndef WATER_METER_H
#define WATER_METER_H

#include "Arduino.h"

#define WATER_METER_PIN 12
#define VALUES_COUNT 10

class WaterMeter {
  public:
    WaterMeter();
    virtual ~WaterMeter();
    void start();
    void stop();
    void calculate();
  private:
    static void isrWaterMeterPulses();
    static void isrTimer();

    static volatile unsigned int pulseCount;
    static volatile unsigned int samplesCount;
    static volatile byte pulsesPos;
    static volatile unsigned int pulsesCount[VALUES_COUNT];
};

#endif

