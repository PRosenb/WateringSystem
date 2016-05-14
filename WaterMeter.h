#ifndef WATER_METER_H
#define WATER_METER_H

#include "Arduino.h"

#define WATER_METER_PIN 12
#define VALUES_COUNT 10

class WaterMeter {
  public:
    WaterMeter(const unsigned long invervalMs);
    virtual ~WaterMeter();
    void start();
    void stop();
    void setThresholdCallback(const unsigned long samplesInInterval, void (*callback)());
    void removeThresholdCallback();
    unsigned int getTotalCount() {
      return totalPulseCount;
    }
  private:
    static void (*callback)();
    static void isrWaterMeterPulses();
    static void isrTimer();

    bool started;
    static volatile unsigned long samplesInInterval;
    static volatile unsigned int totalPulseCount;
    static volatile unsigned int lastPulseCount;
};

#endif

