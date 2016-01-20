
#include <MsTimer2.h>     // https://github.com/PaulStoffregen/MsTimer2

#ifndef PinChangeInt_h
#define LIBCALL_PINCHANGEINT
#include "../PinChangeInt/PinChangeInt.h"
#endif
#include "DeepSleepScheduler.h"

#include "WaterMeter.h"

volatile unsigned int WaterMeter::totalPulseCount;
volatile unsigned int WaterMeter::lastPulseCount;
volatile byte WaterMeter::pulsesPos;
volatile unsigned int WaterMeter::samplesCount;
volatile unsigned int WaterMeter::pulsesCount[VALUES_COUNT];

WaterMeter::WaterMeter() {
  pinMode(WATER_METER_PIN, INPUT_PULLUP);
  MsTimer2::set(100, WaterMeter::isrTimer);
  totalPulseCount = 0;
  started = false;
}

WaterMeter::~WaterMeter() {
}

void WaterMeter::start() {
  if (!started) {
    started = true;
    scheduler.aquireNoDeepSleepLock();
    lastPulseCount = totalPulseCount;
    pulsesPos = 0;
    samplesCount = 0;

    attachPinChangeInterrupt(WATER_METER_PIN, WaterMeter::isrWaterMeterPulses, FALLING);
    MsTimer2::start();
  }
}

void WaterMeter::stop() {
  if (started) {
    started = false;
    MsTimer2::stop();
    detachPinChangeInterrupt(WATER_METER_PIN);
    scheduler.releaseNoDeepSleepLock();
  }
}

void WaterMeter::calculate() {
  unsigned int waterMeterValueSum = 0;

  noInterrupts();
  unsigned int samplesCountLocal = samplesCount;
  samplesCount = 0;
  for (byte i = 0; i < VALUES_COUNT; i++) {
    waterMeterValueSum += pulsesCount[i];
  }
  interrupts();

  Serial.print(F("value: "));
  Serial.print(1.0 * waterMeterValueSum);
  Serial.print(F(", samples: "));
  Serial.println(samplesCountLocal);
}

void WaterMeter::isrWaterMeterPulses() {
  totalPulseCount++;
}

void WaterMeter::isrTimer() {
  pulsesPos %= VALUES_COUNT;
  pulsesCount[pulsesPos] = totalPulseCount - lastPulseCount;
  lastPulseCount = totalPulseCount;
  samplesCount++;
}

