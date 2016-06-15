#ifndef WATER_METER_H
#define WATER_METER_H

#include "Arduino.h"

#define LIBCALL_DEEP_SLEEP_SCHEDULER
#include "DeepSleepScheduler.h"

#define WATER_METER_PIN 12
#define VALUES_COUNT 10

class WaterMeter {
  public:
    WaterMeter(const unsigned long invervalMs);
    virtual ~WaterMeter();
    void start();
    void stop();
    void setThresholdListener(const unsigned long samplesInInterval, Runnable *listener);
    unsigned int getTotalCount() {
      return totalPulseCount;
    }
  private:
    static volatile Runnable *listener;
    static void isrWaterMeterPulses();
    static void isrTimer();

    bool started;
    static volatile unsigned long samplesInInterval;
    static volatile unsigned int totalPulseCount;
    static volatile unsigned int lastPulseCount;
};

#endif

