
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
  sleepMode = SLEEP_MODE_PWR_DOWN;

  // http://www.atmel.com/webdoc/AVRLibcReferenceManual/group__avr__power.html
  //  power_timer0_disable();
  power_timer1_disable();
  power_timer2_disable();
  //  power_twi_disable();
  power_usart0_disable();
  power_adc_enable();
  power_spi_disable();

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
  awakeLed->off();
  delete awakeLed;
}

void SleepManager::setSleepMode(byte sleepMode) {
  SleepManager::sleepMode = sleepMode;
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

// with DS3231 and MOSFET4 connected, no MOSFET on
// power supply not connected: 1.40W
// SLEEP_MODE_PWR_DOWN: 18.5mA, 1.8W
// ON: 54.2mA
// ON but timer1/2, usart, adc, spi OFF: 50.5mA, 2.45W
// SLEEP_MODE_IDLE with timer0/1/2, usart, adc, spi, twi OFF: 38.5mA
// SLEEP_MODE_IDLE with timer0/2, usart, adc, spi, twi OFF: 41.0mA, 2.25W
void SleepManager::sleepNow() {
  Serial.end();
  awakeLed->off();
  
  power_timer0_disable();
  power_twi_disable();
  set_sleep_mode(sleepMode);
  // sleep is only be done if sleep_enable() is executed before
  sleep_mode();            // here the device is actually put to sleep!!
  // THE PROGRAM CONTINUES FROM HERE AFTER WAKING UP
  power_timer0_enable();
  power_twi_enable();
  
  awakeLed->on();
  Serial.begin(9600);
}

