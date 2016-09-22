
#include <DS3232RTC.h>    // http://github.com/JChristensen/DS3232RTC
#include "Constants.h"

#define AWAKE_INDICATION_PIN DEEP_SLEEP_SCHEDULER_AWAKE_INDICATION_PIN
#define DEEP_SLEEP_DELAY 100
#define SUPERVISION_CALLBACK
#include "DeepSleepScheduler.h"

#define EI_NOTPORTC
#define EI_NOTPORTD
#include <EnableInterrupt.h> // https://github.com/GreyGnome/EnableInterrupt
#include "EEPROMWearLevel.h"

#include "WaterManager.h"
#include "SerialManager.h"

SerialManager *serialManager;
WaterManager *waterManager;

class SupervisionCallback: public Runnable {
    void run() {
      int resetCount = 0;
      EEPROMwl.get(EEPROM_INDEX_WATCHDOG_RESET_COUNT, resetCount);
      EEPROMwl.put(EEPROM_INDEX_WATCHDOG_RESET_COUNT, resetCount + 1);
    }
};

void setup() {
  serialManager = new SerialManager(BLUETOOTH_ENABLE_PIN);
  EEPROMwl.begin(EEPROM_VERSION, EEPROM_INDEX_COUNT, EEPROM_LENGTH_TO_USE);
  waterManager = new WaterManager();
  serialManager->setWaterManager(waterManager);

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

  scheduler.setSupervisionCallback(new SupervisionCallback());

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
  serialManager->startSerial(SERIAL_SLEEP_TIMEOUT_MS);
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
  serialManager->startSerial(SERIAL_SLEEP_TIMEOUT_MS);
}

void isrStartManual() {
  scheduler.schedule(startManual);
}

void startAutomatic() {
  Serial.println(F("startAutomatic"));
  waterManager->startAutomatic();
  serialManager->startSerial(SERIAL_SLEEP_TIMEOUT_MS);

  // simple debounce
  delay(200);
  scheduler.removeCallbacks(startAutomatic);
}

void isrStartAutomatic() {
  scheduler.schedule(startAutomatic);
}

// ----------------------------------------------------------------------------------
// helper
// ----------------------------------------------------------------------------------


