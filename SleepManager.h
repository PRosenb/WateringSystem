
#ifndef SLEEP_MANAGER_H
#define SLEEP_MANAGER_H

#include "Arduino.h"

// http://arduino-info.wikispaces.com/HAL-LibrariesUpdates
#include <LED.h>

#define UNUSED 255

// potential PinChangePins on Leonardo: 8, 9, 10, 11
#define RTC_INT_PIN 8
#define WAKEUP_INTERRUPT_1_PIN 9
#define WAKEUP_INTERRUPT_2_PIN 10
#define WAKEUP_INTERRUPT_3_PIN 11

#define AWAKE_LED_PIN 13

class Interrupts {
  public:
    unsigned int wakeupInterrupt1=0, wakeupInterrupt2=0, wakeupInterrupt3=0, rtcAlarm1=0, rtcAlarm2=0;
};

class SleepManager {
  public:
    SleepManager();
    virtual ~SleepManager();
    // SLEEP_MODE_IDLE         - the least power savings, modules on
    // SLEEP_MODE_ADC
    // SLEEP_MODE_PWR_SAVE
    // SLEEP_MODE_STANDBY
    // SLEEP_MODE_PWR_DOWN     - the most power savings
    void setSleepMode(byte sleepMode);
    Interrupts sleep();
    Interrupts getSeenInterruptsAndClear();
  private:
    static void isrRtc();
    static void isrWakeupInterrupt1();
    static void isrWakeupInterrupt2();
    static void isrWakeupInterrupt3();
    void sleepNow();
    byte sleepMode;
    LED *awakeLed;
    static volatile boolean wakeupInterruptRtc;
    static volatile unsigned int wakeupInterrupt1Count;
    static volatile unsigned int wakeupInterrupt2Count;
    static volatile unsigned int wakeupInterrupt3Count;
};

#endif

