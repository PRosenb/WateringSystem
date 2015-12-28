
#include "SleepManager.h"
#include <PinChangeInt.h>
#include <DS3232RTC.h>    //http://github.com/JChristensen/DS3232RTC

#include <avr/sleep.h>
#include <avr/power.h>

volatile unsigned int SleepManager::wakeupInterrupt1Count;
volatile unsigned int SleepManager::wakeupInterrupt2Count;
volatile unsigned int SleepManager::wakeupInterrupt3Count;

SleepManager::SleepManager() {
  wakeupInterrupt1Count = 0;
  wakeupInterrupt2Count = 0;
  wakeupInterrupt3Count = 0;
  awakeLed = new LED(AWAKE_LED_PIN);

  pinMode(RTC_INT_PIN, INPUT_PULLUP);
  pinMode(WAKEUP_INTERRUPT_1_PIN, INPUT_PULLUP);
  pinMode(WAKEUP_INTERRUPT_2_PIN, INPUT_PULLUP);
  pinMode(WAKEUP_INTERRUPT_3_PIN, INPUT_PULLUP);
  attachPinChangeInterrupt(RTC_INT_PIN, SleepManager::isrRtc, FALLING);
  attachPinChangeInterrupt(WAKEUP_INTERRUPT_1_PIN, SleepManager::isrWakeupInterrupt1, FALLING);
  attachPinChangeInterrupt(WAKEUP_INTERRUPT_2_PIN, SleepManager::isrWakeupInterrupt2, FALLING);
  attachPinChangeInterrupt(WAKEUP_INTERRUPT_3_PIN, SleepManager::isrWakeupInterrupt3, FALLING);

  setSyncProvider(RTC.get);
  awakeLed->on();
}

SleepManager::~SleepManager() {
  delete awakeLed;
}

Interrupts SleepManager::sleep() {
  delay(100); // allow time for serial write

  // do sleep_enable()/sleep_disable() before/after the checks to prevent race condition with
  // sleep_disable() in the isr methods
  sleep_enable();          // enables the sleep bit, a safety pin
  unsigned int rtcAlarm1Count = RTC.alarm(ALARM_1) ? 1 : 0;
  unsigned int rtcAlarm2Count = RTC.alarm(ALARM_2) ? 1 : 0;

  noInterrupts();
  unsigned int wakeupInterrupt1CountLocal = wakeupInterrupt1Count;
  unsigned int wakeupInterrupt2CountLocal = wakeupInterrupt2Count;
  unsigned int wakeupInterrupt3CountLocal = wakeupInterrupt3Count;
  interrupts();

  while (wakeupInterrupt1CountLocal == 0 && wakeupInterrupt2CountLocal == 0 && wakeupInterrupt3CountLocal == 0
         && rtcAlarm1Count == 0 && rtcAlarm2Count == 0) {
    sleepNow();
    if (RTC.alarm(ALARM_1)) rtcAlarm1Count++;
    if (RTC.alarm(ALARM_2)) rtcAlarm2Count++;

    noInterrupts();
    wakeupInterrupt1CountLocal = wakeupInterrupt1Count;
    wakeupInterrupt2CountLocal = wakeupInterrupt2Count;
    wakeupInterrupt3CountLocal = wakeupInterrupt3Count;
    interrupts();
  }
  sleep_disable();

  // set the provider and do as sync now
  setSyncProvider(RTC.get);

  Interrupts interrupts = getSeenInterruptsAndClear();
  interrupts.rtcAlarm1 += rtcAlarm1Count;
  interrupts.rtcAlarm2 += rtcAlarm2Count;

  return interrupts;
}

Interrupts SleepManager::getSeenInterruptsAndClear() {
  Interrupts interrupts;
  interrupts.rtcAlarm1 = RTC.alarm(ALARM_1);
  interrupts.rtcAlarm2 = RTC.alarm(ALARM_2);

  noInterrupts();
  interrupts.wakeupInterrupt1 = wakeupInterrupt1Count;
  interrupts.wakeupInterrupt2 = wakeupInterrupt2Count;
  interrupts.wakeupInterrupt3 = wakeupInterrupt3Count;
  wakeupInterrupt1Count = 0;
  wakeupInterrupt2Count = 0;
  wakeupInterrupt3Count = 0;
  interrupts();

  return interrupts;
}

void SleepManager::isrRtc() {
  // for the case when the interrupt executes directly before sleep starts
  sleep_disable();
}
void SleepManager::isrWakeupInterrupt1() {
  // for the case when the interrupt executes directly before sleep starts
  sleep_disable();
  wakeupInterrupt1Count++;
}
void SleepManager::isrWakeupInterrupt2() {
  // for the case when the interrupt executes directly before sleep starts
  sleep_disable();
  wakeupInterrupt2Count++;
}
void SleepManager::isrWakeupInterrupt3() {
  // for the case when the interrupt executes directly before sleep starts
  sleep_disable();
  wakeupInterrupt3Count++;
}

// sleep: 19.5mA, on 54.2mA
void SleepManager::sleepNow() {
  Serial.end();
  //  attachPinChangeInterrupt(RTC_INT_PIN, SleepManager::isrRtc, FALLING);
  awakeLed->off();
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
  awakeLed->on();
  //  detachPinChangeInterrupt(RTC_INT_PIN);
  Serial.begin(9600);
}

