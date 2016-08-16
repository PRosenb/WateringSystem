
#include <DS3232RTC.h>    // http://github.com/JChristensen/DS3232RTC

#define AWAKE_INDICATION_PIN 13
#define DEEP_SLEEP_DELAY 100
#include "DeepSleepScheduler.h"

#define EI_NOTPORTC
#define EI_NOTPORTD
#include <EnableInterrupt.h> // https://github.com/GreyGnome/EnableInterrupt

#include "WaterManager.h"
#include "SerialManager.h"

// potential PinChangePins on Leonardo: 8, 9, 10, 11
#define RTC_INT_PIN 8
#define START_MANUAL_PIN 9
#define START_AUTOMATIC_PIN 10
#define MODE_PIN 11
#define BLUETOOTH_ENABLE_PIN 2

SerialManager *serialManager;
WaterManager *waterManager;

void setup() {
  waterManager = new WaterManager();
  serialManager = new SerialManager(waterManager, BLUETOOTH_ENABLE_PIN);

  RTC.alarmInterrupt(ALARM_1, true);
  RTC.alarmInterrupt(ALARM_2, false);
  // reset alarms if active
  RTC.alarm(ALARM_1);
  RTC.alarm(ALARM_2);
  delay(1000);

  pinMode(RTC_INT_PIN, INPUT_PULLUP);
  enableInterrupt(RTC_INT_PIN, isrRtc, FALLING);
  //  pinMode(START_MANUAL_PIN, INPUT_PULLUP);
  //  enableInterrupt(START_MANUAL_PIN, isrStartManual, FALLING);
  pinMode(START_AUTOMATIC_PIN, INPUT_PULLUP);
  enableInterrupt(START_AUTOMATIC_PIN, isrStartAutomatic, FALLING);
  pinMode(MODE_PIN, INPUT_PULLUP);
  enableInterrupt(MODE_PIN, isrMode, FALLING);

  serialManager->startSerial(SERIAL_SLEEP_TIMEOUT_MS);
}

void loop() {
  scheduler.execute();
}

// ----------------------------------------------------------------------------------
// interrupts
// ----------------------------------------------------------------------------------
void modeScheduled() {
  waterManager->modeClicked();
  serialManager->startSerial(SERIAL_SLEEP_TIMEOUT_MS);

  // simple debounce
  delay(200);
  scheduler.removeCallbacks(modeScheduled);
}

void isrMode() {
  if (!scheduler.isScheduled(modeScheduled)) {
    scheduler.schedule(modeScheduled);
  }
}

void startAutomaticRtc() {
  Serial.println(F("startAutomaticRtc"));
  waterManager->startAutomaticRtc();
}

void rtcScheduled() {
  //  Serial.println(F("rtcScheduled"));delay(150);
  if (RTC.alarm(ALARM_1)) {
    scheduler.schedule(startAutomaticRtc);
  }
  if (RTC.alarm(ALARM_2)) {
    // not used
  }
}

void isrRtc() {
  scheduler.schedule(rtcScheduled);
}

void startManual() {
  Serial.println(F("startManual"));
  waterManager->startManual();
}

void isrStartManual() {
  scheduler.schedule(startManual);
}

void startAutomatic() {
  Serial.println(F("startAutomatic"));
  waterManager->startAutomatic();
}

void isrStartAutomatic() {
  scheduler.schedule(startAutomatic);
}

// ----------------------------------------------------------------------------------
// helper
// ----------------------------------------------------------------------------------


