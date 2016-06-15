
#include <MsTimer2.h>     // https://github.com/PaulStoffregen/MsTimer2

#define LIBCALL_ENABLEINTERRUPT
#include <EnableInterrupt.h>

#define LIBCALL_DEEP_SLEEP_SCHEDULER
#include "DeepSleepScheduler.h"

#include "WaterMeter.h"

volatile unsigned long WaterMeter::samplesInInterval;
volatile unsigned int WaterMeter::totalPulseCount;
volatile unsigned int WaterMeter::lastPulseCount;
volatile Runnable *WaterMeter::listener;

WaterMeter::WaterMeter(const unsigned long invervalMs) {
  pinMode(WATER_METER_PIN, INPUT_PULLUP);
  MsTimer2::set(invervalMs, WaterMeter::isrTimer);
  totalPulseCount = 0;
  started = false;
}

WaterMeter::~WaterMeter() {
}

void WaterMeter::start() {
  if (!started) {
    started = true;
    scheduler.acquireNoDeepSleepLock();
    lastPulseCount = totalPulseCount;

    enableInterrupt(WATER_METER_PIN, WaterMeter::isrWaterMeterPulses, FALLING);
    MsTimer2::start();
  }
}

void WaterMeter::stop() {
  if (started) {
    started = false;
    MsTimer2::stop();
    disableInterrupt(WATER_METER_PIN);
    scheduler.releaseNoDeepSleepLock();
  }
}

void WaterMeter::setThresholdListener(const unsigned long samplesInInterval, Runnable *runnable) {
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
    scheduler.schedule((Runnable*) listener);
  }
}

