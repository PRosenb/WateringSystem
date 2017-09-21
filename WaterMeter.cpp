
#define LIBCALL_ENABLEINTERRUPT
#include <EnableInterrupt.h>

#define LIBCALL_DEEP_SLEEP_SCHEDULER
#include <DeepSleepScheduler.h> // https://github.com/PRosenb/DeepSleepScheduler

#include "WaterMeter.h"

volatile unsigned int WaterMeter::totalPulseCount;
volatile unsigned int WaterMeter::lastPulseCount;
volatile unsigned int WaterMeter::samplesInInterval;
volatile unsigned int WaterMeter::lastPulseCountOverThreshold;
volatile Runnable *WaterMeter::listener;

WaterMeter::WaterMeter(const unsigned long invervalMs) {
  pinMode(WATER_METER_PIN, INPUT_PULLUP);
  MsTimer2::set(invervalMs, WaterMeter::isrTimer);
  totalPulseCount = 0;
  lastPulseCountOverThreshold = 0;
  started = false;
}

WaterMeter::~WaterMeter() {
}

void WaterMeter::start() {
  if (!started) {
    started = true;
    scheduler.acquireNoDeepSleepLock();

    enableInterrupt(WATER_METER_PIN, WaterMeter::isrWaterMeterPulses, FALLING);
    if (thresholdSupervisionDelay == 0) {
      lastPulseCount = totalPulseCount;
      MsTimer2::start();
    } else {
      scheduler.scheduleDelayed(this, thresholdSupervisionDelay);
    }
  }
}

void WaterMeter::stop() {
  if (started) {
    started = false;
    MsTimer2::stop();
    disableInterrupt(WATER_METER_PIN);
    scheduler.releaseNoDeepSleepLock();
    scheduler.removeCallbacks(this);
  }
}

void WaterMeter::run() {
  if (started) {
    lastPulseCount = totalPulseCount;
    MsTimer2::start();
  }
}

void WaterMeter::setThresholdListener(const unsigned int samplesInInterval, Runnable *listener) {
  WaterMeter::samplesInInterval = samplesInInterval;
  WaterMeter::listener = listener;
}

void WaterMeter::isrWaterMeterPulses() {
  totalPulseCount++;
}

void WaterMeter::isrTimer() {
  unsigned int pulsesCount = totalPulseCount - lastPulseCount;
  lastPulseCount = totalPulseCount;
  if (listener != NULL && pulsesCount >= samplesInInterval) {
    lastPulseCountOverThreshold = pulsesCount;
    scheduler.schedule((Runnable*) listener);
  }
}

