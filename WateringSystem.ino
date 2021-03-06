
#include <DS3232RTC.h>    // http://github.com/JChristensen/DS3232RTC
#include "Constants.h"

#define AWAKE_INDICATION_PIN DEEP_SLEEP_SCHEDULER_AWAKE_INDICATION_PIN
#define DEEP_SLEEP_DELAY 100
#define SUPERVISION_CALLBACK
#include <DeepSleepScheduler.h> // https://github.com/PRosenb/DeepSleepScheduler

#define EI_NOTPORTC
#define EI_NOTPORTD
#include <EnableInterrupt.h> // https://github.com/GreyGnome/EnableInterrupt
#include <EEPROMWearLevel.h> // https://github.com/PRosenb/EEPROMWearLevel

#include "WaterManager.h"
#include "SerialManager.h"

SerialManager *serialManager;
WaterManager *waterManager;

void setup() {
  if (!superviseCrashResetCount()) {
    // too many resets
    return;
  }

  serialManager = new SerialManager(BLUETOOTH_ENABLE_PIN);
  waterManager = new WaterManager();
  serialManager->setWaterManager(waterManager);

  initRtc();

  pinMode(START_AUTOMATIC_PIN, INPUT_PULLUP);
  enableInterrupt(START_AUTOMATIC_PIN, isrStartAutomatic, FALLING);
  pinMode(MODE_PIN, INPUT_PULLUP);
  enableInterrupt(MODE_PIN, isrMode, FALLING);

  serialManager->startSerial();
}

void loop() {
  scheduler.execute();
}

// ----------------------------------------------------------------------------------
// reset count supervision
// ----------------------------------------------------------------------------------
/**
   Defines the maximum numbers of resets caused by a crash. This number is not
   reset. If the problem is fixed it is best to increase this number e.g. to 200.
*/
#define MAX_RESET_COUNT 100

class SupervisionCallback: public Runnable {
    void run() {
      int resetCount = 0;
      EEPROMwl.get(EEPROM_INDEX_WATCHDOG_RESET_COUNT, resetCount);
      EEPROMwl.put(EEPROM_INDEX_WATCHDOG_RESET_COUNT, resetCount + 1);
    }
};

/**
   return false if not too many crash resets, true if system should stop.
*/
inline bool superviseCrashResetCount() {
  EEPROMwl.begin(EEPROM_VERSION, EEPROM_INDEX_COUNT, EEPROM_LENGTH_TO_USE);
  int resetCount = 0;
  EEPROMwl.get(EEPROM_INDEX_WATCHDOG_RESET_COUNT, resetCount);
  if (resetCount > MAX_RESET_COUNT) {
    // too many crashes, prevent execution
    pinMode(COLOR_LED_GREEN_PIN, OUTPUT);
    digitalWrite(COLOR_LED_GREEN_PIN, LOW);
    pinMode(COLOR_LED_RED_PIN, OUTPUT);
    digitalWrite(COLOR_LED_RED_PIN, HIGH);
    pinMode(COLOR_LED_BLUE_PIN, OUTPUT);
    digitalWrite(COLOR_LED_BLUE_PIN, LOW);

    Serial.begin(9600);
    // wait for serial port to connect
    while (!Serial);
    Serial.println(F("too many resets"));

    return false;
  } else {
    // no problem, execute as normal
    scheduler.setSupervisionCallback(new SupervisionCallback());
    return true;
  }
}

// ----------------------------------------------------------------------------------
// RTC initialisation
// ----------------------------------------------------------------------------------
inline void initRtc() {
  // reset alarms if active
  RTC.alarm(ALARM_1);
  RTC.alarm(ALARM_2);
  delay(1000);

  pinMode(RTC_INT_PIN, INPUT_PULLUP);
  enableInterrupt(RTC_INT_PIN, isrRtc, FALLING);
}

// ----------------------------------------------------------------------------------
// interrupts and their callback methods
// ----------------------------------------------------------------------------------
void modeScheduled() {
  waterManager->modeClicked();
  serialManager->startSerial();

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
  serialManager->startSerial();
}

void rtcScheduled() {
  //  Serial.println(F("rtcScheduled"));delay(150);
  if (RTC.alarm(ALARM_1)) {
    scheduler.schedule(startAutomaticRtc);
  }
  if (RTC.alarm(ALARM_2)) {
    scheduler.schedule(startAutomaticRtc);
  }
}

void isrRtc() {
  scheduler.schedule(rtcScheduled);
}

void startAutomatic() {
  Serial.println(F("startAutomatic"));
  waterManager->startAutomatic();
  serialManager->startSerial();

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


