
#ifndef SLEEP_MANAGER_H
#define SLEEP_MANAGER_H

#include "Arduino.h"

// http://arduino-info.wikispaces.com/HAL-LibrariesUpdates
#include <LED.h>

// potential PinChangePins on Leonardo: 8, 9, 10, 11
#define RTC_INT_PIN 8
#define WAKEUP_INTERRUPT_1_PIN 9
#define WAKEUP_INTERRUPT_2_PIN 10
#define WAKEUP_INTERRUPT_3_PIN 11

#define AWAKE_LED_PIN 13

class Interrupts {
  public:
    unsigned int wakeupInterrupt1, wakeupInterrupt2, wakeupInterrupt3, rtcAlarm1, rtcAlarm2;
};

class SleepManager {
  public:
    SleepManager();
    virtual ~SleepManager();
    Interrupts sleep();
    Interrupts getSeenInterruptsAndClear();
  private:
    static void isrRtc();
    static void isrWakeupInterrupt1();
    static void isrWakeupInterrupt2();
    static void isrWakeupInterrupt3();
    void sleepNow();
    LED *awakeLed;
    static volatile unsigned int wakeupInterrupt1Count;
    static volatile unsigned int wakeupInterrupt2Count;
    static volatile unsigned int wakeupInterrupt3Count;
};

#endif

