
#include "SleepManager.h"
#include <PinChangeInt.h>
#include <DS3232RTC.h>    //http://github.com/JChristensen/DS3232RTC

#include <avr/sleep.h>
#include <avr/power.h>

volatile boolean SleepManager::button1Triggered;
volatile boolean SleepManager::button2Triggered;
volatile boolean SleepManager::button3Triggered;

SleepManager::SleepManager() {
  button1Triggered = false;
  button2Triggered = false;
  button3Triggered = false;

  pinMode(BUTTON_1_INT_PIN, INPUT_PULLUP);
  pinMode(BUTTON_2_INT_PIN, INPUT_PULLUP);
  pinMode(BUTTON_3_INT_PIN, INPUT_PULLUP);
  pinMode(RTC_INT_PIN, INPUT_PULLUP);
  attachPinChangeInterrupt(RTC_INT_PIN, SleepManager::isrRtc, FALLING);
  attachPinChangeInterrupt(BUTTON_1_INT_PIN, SleepManager::isrButton1, FALLING);
  attachPinChangeInterrupt(BUTTON_2_INT_PIN, SleepManager::isrButton2, FALLING);
  attachPinChangeInterrupt(BUTTON_3_INT_PIN, SleepManager::isrButton3, FALLING);

  setSyncProvider(RTC.get);
  awakeLed.on();
}

Interrupts SleepManager::sleep() {
  delay(100); // allow time for serial write

  // do sleep_enable()/sleep_disable() before/after the checks to prevent race condition with
  // sleep_disable() in the isr methods
  sleep_enable();          // enables the sleep bit, a safety pin
  boolean rtcAlarm1Triggered = RTC.alarm(ALARM_1);
  boolean rtcAlarm2Triggered = RTC.alarm(ALARM_2);
  while (!button1Triggered && !button2Triggered && !button3Triggered && !rtcAlarm1Triggered && !rtcAlarm2Triggered) {
    sleepNow();
    rtcAlarm1Triggered = RTC.alarm(ALARM_1);
    rtcAlarm2Triggered = RTC.alarm(ALARM_2);
  }
  sleep_disable();

  // set the provider and do as sync now
  setSyncProvider(RTC.get);

  Interrupts interrupts = getSeenInterruptsAndClear();
  if (rtcAlarm1Triggered) {
    interrupts.rtcAlarm1 = true;
  }
  if (rtcAlarm2Triggered) {
    interrupts.rtcAlarm2 = true;
  }

  return interrupts;
}

Interrupts SleepManager::getSeenInterruptsAndClear() {
  Interrupts interrupts;
  interrupts.button1 = button1Triggered;
  interrupts.button2 = button2Triggered;
  interrupts.button3 = button3Triggered;
  interrupts.rtcAlarm1 = RTC.alarm(ALARM_1);
  interrupts.rtcAlarm2 = RTC.alarm(ALARM_2);

  button1Triggered = false;
  button2Triggered = false;
  button3Triggered = false;

  return interrupts;
}

void SleepManager::isrRtc() {
  // for the case when the interrupt executes directly before sleep starts
  sleep_disable();
  digitalWrite(7, HIGH);
}
void SleepManager::isrButton1() {
  // for the case when the interrupt executes directly before sleep starts
  sleep_disable();
  button1Triggered = true;
}
void SleepManager::isrButton2() {
  // for the case when the interrupt executes directly before sleep starts
  sleep_disable();
  button2Triggered = true;
}
void SleepManager::isrButton3() {
  // for the case when the interrupt executes directly before sleep starts
  sleep_disable();
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
  power_all_disable();
  // sleep is only be done if sleep_enable() is executed before
  sleep_mode();            // here the device is actually put to sleep!!
  // THE PROGRAM CONTINUES FROM HERE AFTER WAKING UP
  power_all_enable();
  awakeLed.on();
  //  detachPinChangeInterrupt(RTC_INT_PIN);
  Serial.begin(9600);
}

