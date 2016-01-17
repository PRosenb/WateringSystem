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
    unsigned int getTotalCount() {
      return totalPulseCount;
    }
    void calculate();
  private:
    static void isrWaterMeterPulses();
    static void isrTimer();

    bool started;
    static volatile unsigned int totalPulseCount;
    static volatile unsigned int lastPulseCount;
    static volatile unsigned int samplesCount;
    static volatile byte pulsesPos;
    static volatile unsigned int pulsesCount[VALUES_COUNT];
};

#endif

