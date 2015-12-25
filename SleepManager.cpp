
#include "SleepManager.h"
#include <PinChangeInt.h>
#include <DS3232RTC.h>    //http://github.com/JChristensen/DS3232RTC

#include <avr/sleep.h>
#include <avr/power.h>

volatile boolean SleepManager::button1Triggered;
volatile boolean SleepManager::button2Triggered;
volatile boolean SleepManager::button3Triggered;

SleepManager::SleepManager() {
  attachPinChangeInterrupt(RTC_INT_PIN, SleepManager::isrRtc, FALLING);
  setSyncProvider(RTC.get);

  button1Triggered = false;
  button2Triggered = false;
  button3Triggered = false;
  awakeLed.on();

  pinMode(BUTTON_1_INT_PIN, INPUT_PULLUP);
  pinMode(BUTTON_2_INT_PIN, INPUT_PULLUP);
  pinMode(BUTTON_3_INT_PIN, INPUT_PULLUP);
  pinMode(RTC_INT_PIN, INPUT_PULLUP);

  attachPinChangeInterrupt(BUTTON_1_INT_PIN, SleepManager::isrButton1, FALLING);
  attachPinChangeInterrupt(BUTTON_2_INT_PIN, SleepManager::isrButton2, FALLING);
  attachPinChangeInterrupt(BUTTON_3_INT_PIN, SleepManager::isrButton3, FALLING);
}

CausingInterrupt SleepManager::sleep() {
  boolean rtcAlarm1Triggered;
  boolean rtcAlarm2Triggered;
  delay(100); // allow time for serial write
  do {
    sleepNow();
    rtcAlarm1Triggered = RTC.alarm(ALARM_1);
    rtcAlarm2Triggered = RTC.alarm(ALARM_2);
  } while (!button1Triggered && !button2Triggered && !button3Triggered && !rtcAlarm1Triggered && !rtcAlarm2Triggered);

  // set the provider and do as sync now
  setSyncProvider(RTC.get);

  CausingInterrupt causingInterrupt = UNKNOWN_INT;
  if (button1Triggered) {
    causingInterrupt = BUTTON_1_INT;
  } else if (button2Triggered) {
    causingInterrupt = BUTTON_2_INT;
  } else if (button3Triggered) {
    causingInterrupt = BUTTON_3_INT;
  } else if (rtcAlarm1Triggered) {
    causingInterrupt = RTC_ALARM_1_INT;
  } else if (rtcAlarm2Triggered) {
    causingInterrupt = RTC_ALARM_2_INT;
  }
  button1Triggered = false;
  button2Triggered = false;
  button3Triggered = false;
  return causingInterrupt;
}

boolean SleepManager::isInterruptAndReset(CausingInterrupt causingInterrupt) {
  boolean didInterrupt = false;
  switch (causingInterrupt) {
    case BUTTON_1_INT:
      didInterrupt = button1Triggered;
      button1Triggered = false;
      break;
    case BUTTON_2_INT:
      didInterrupt = button2Triggered;
      button2Triggered = false;
      break;
    case BUTTON_3_INT:
      didInterrupt = button3Triggered;
      button3Triggered = false;
      break;
  }
  return didInterrupt;
}

void SleepManager::isrRtc() {
  digitalWrite(7, HIGH);
  // wake up only, check rtc if it has alarm
}
void SleepManager::isrButton1() {
  button1Triggered = true;
}
void SleepManager::isrButton2() {
  button2Triggered = true;
}
void SleepManager::isrButton3() {
  button3Triggered = true;
}

// sleep: 19.5mA, on 54.2mA
void SleepManager::sleepNow() {
  Serial.end();
  //  attachPinChangeInterrupt(RTC_INT_PIN, SleepManager::isrRtc, FALLING);
  awakeLed.off();
  // SLEEP_MODE_IDLE         - the least power savings
  // SLEEP_MODE_ADC
  // SLEEP_MODE_PWR_SAVE
  // SLEEP_MODE_STANDBY
  // SLEEP_MODE_PWR_DOWN     - the most power savings
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();          // enables the sleep bit, just a safety pin
  power_all_disable();
  sleep_mode();            // here the device is actually put to sleep!!
  // THE PROGRAM CONTINUES FROM HERE AFTER WAKING UP
  sleep_disable();         // first thing after waking from sleep: disable sleep...
  power_all_enable();
  awakeLed.on();
  //  detachPinChangeInterrupt(RTC_INT_PIN);
  Serial.begin(9600);
}

