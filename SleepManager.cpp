
#include "SleepManager.h"
#include <PinChangeInt.h> // https://github.com/GreyGnome/PinChangeInt
#include <DS3232RTC.h>    // http://github.com/JChristensen/DS3232RTC

#include <avr/sleep.h>
#include <avr/power.h>

volatile boolean SleepManager::wakeupInterruptRtc;
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

#if RTC_INT_PIN != UNUSED
  pinMode(RTC_INT_PIN, INPUT_PULLUP);
  attachPinChangeInterrupt(RTC_INT_PIN, SleepManager::isrRtc, FALLING);
#endif
#if WAKEUP_INTERRUPT_1_PIN != UNUSED
  pinMode(WAKEUP_INTERRUPT_1_PIN, INPUT_PULLUP);
  attachPinChangeInterrupt(WAKEUP_INTERRUPT_1_PIN, SleepManager::isrWakeupInterrupt1, FALLING);
#endif
#if WAKEUP_INTERRUPT_2_PIN != UNUSED
  pinMode(WAKEUP_INTERRUPT_2_PIN, INPUT_PULLUP);
  attachPinChangeInterrupt(WAKEUP_INTERRUPT_2_PIN, SleepManager::isrWakeupInterrupt2, FALLING);
#endif
#if WAKEUP_INTERRUPT_3_PIN != UNUSED
  pinMode(WAKEUP_INTERRUPT_3_PIN, INPUT_PULLUP);
  attachPinChangeInterrupt(WAKEUP_INTERRUPT_3_PIN, SleepManager::isrWakeupInterrupt3, FALLING);
#endif

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
  Interrupts interrupts = getSeenInterruptsAndClear();

  while (interrupts.wakeupInterrupt1 == 0 && interrupts.wakeupInterrupt2 == 0 && interrupts.wakeupInterrupt3 == 0
         && interrupts.rtcAlarm1 == 0 && interrupts.rtcAlarm2 == 0) {
    sleepNow();

    interrupts = getSeenInterruptsAndClear();
  }
  sleep_disable();

  // set the provider and do as sync now
  setSyncProvider(RTC.get);
  return interrupts;
}

Interrupts SleepManager::getSeenInterruptsAndClear() {
  Interrupts interrupts;

  noInterrupts();
  boolean wakeupInterruptRtcLocal = wakeupInterruptRtc;
  interrupts.wakeupInterrupt1 = wakeupInterrupt1Count;
  interrupts.wakeupInterrupt2 = wakeupInterrupt2Count;
  interrupts.wakeupInterrupt3 = wakeupInterrupt3Count;
  wakeupInterruptRtc = false;
  wakeupInterrupt1Count = 0;
  wakeupInterrupt2Count = 0;
  wakeupInterrupt3Count = 0;
  interrupts();

  // call RTC.alarm in any case to reset alarms but
  // consider rtc alarms only if rtc interrupt occured
  if (RTC.alarm(ALARM_1)) {
    if (wakeupInterruptRtcLocal) {
      interrupts.rtcAlarm1++;
    }
  }
  if (RTC.alarm(ALARM_2)) {
    if (wakeupInterruptRtcLocal) {
      interrupts.rtcAlarm2++;
    }
  }
  return interrupts;
}

void SleepManager::isrRtc() {
  // for the case when the interrupt executes directly before sleep starts
  sleep_disable();
  wakeupInterruptRtc = true;
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

