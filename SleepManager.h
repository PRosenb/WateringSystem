
#ifndef SLEEP_MANAGER_H
#define SLEEP_MANAGER_H

#include "Arduino.h"

// http://arduino-info.wikispaces.com/HAL-LibrariesUpdates
#include <LED.h>

// potential PinChangePins on Leonardo: 8, 9, 10, 11
#define RTC_INT_PIN 8
#define BUTTON_1_INT_PIN 9
#define BUTTON_2_INT_PIN 10

#define AWAKE_LED_PIN 13

enum CausingInterrupt { BUTTON_1_INT, BUTTON_2_INT, RTC_ALARM_1_INT, RTC_ALARM_2_INT, UNKNOWN_INT };

class SleepManager {
  public:
    SleepManager();
    CausingInterrupt sleep();
    boolean isInterruptAndReset(CausingInterrupt causingInterrupt);
  private:
    static void isrRtc();
    static void isrButton1();
    static void isrButton2();
    void sleepNow();
    LED awakeLed = LED(AWAKE_LED_PIN);
    static volatile boolean button1Triggered;
    static volatile boolean button2Triggered;
};

#endif

