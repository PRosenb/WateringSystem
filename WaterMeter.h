#ifndef WATER_METER_H
#define WATER_METER_H

#include "Arduino.h"
#include <MsTimer2.h>     // https://github.com/PaulStoffregen/MsTimer2

#define LIBCALL_DEEP_SLEEP_SCHEDULER
#include <DeepSleepScheduler.h> // https://github.com/PRosenb/DeepSleepScheduler
#include "Constants.h"

#define VALUES_COUNT 10

// Runnable used to delay threshold
class WaterMeter: public Runnable {
  public:
    WaterMeter(const unsigned long invervalMs);
    virtual ~WaterMeter();
    void start();
    void stop();
    void setThresholdListener(const unsigned int samplesInInterval, Runnable *listener);
    inline void setThresholdSupervisionDelay(const unsigned long thresholdSupervisionDelay) {
      WaterMeter::thresholdSupervisionDelay = thresholdSupervisionDelay;
    }
    unsigned int getSamplesInInterval() {
      return samplesInInterval;
    }
    inline unsigned long getTotalCount() {
      return totalPulseCount;
    }
    inline unsigned int getLastPulseCountOverThreshold() {
      return lastPulseCountOverThreshold;
    }
    void run();
  private:
    static volatile Runnable *listener;
    static void isrWaterMeterPulses();
    static void isrTimer();

    bool started;
    unsigned long thresholdSupervisionDelay = 0;
    static volatile unsigned int samplesInInterval;
    static volatile unsigned long totalPulseCount;
    static volatile unsigned long lastPulseCount;
    static volatile unsigned int lastPulseCountOverThreshold;
};

#endif

