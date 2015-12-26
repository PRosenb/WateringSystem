
#ifndef SLEEP_MANAGER_H
#define SLEEP_MANAGER_H

#include "Arduino.h"

// http://arduino-info.wikispaces.com/HAL-LibrariesUpdates
#include <LED.h>

// potential PinChangePins on Leonardo: 8, 9, 10, 11
#define RTC_INT_PIN 8
#define BUTTON_1_INT_PIN 9
#define BUTTON_2_INT_PIN 10
#define BUTTON_3_INT_PIN 11

#define AWAKE_LED_PIN 13

class Interrupts {
  public:
    boolean button1, button2, button3, rtcAlarm1, rtcAlarm2;
};

class SleepManager {
  public:
    SleepManager();
    Interrupts sleep();
    Interrupts getSeenInterruptsAndClear();
  private:
    static void isrRtc();
    static void isrButton1();
    static void isrButton2();
    static void isrButton3();
    void sleepNow();
    LED awakeLed = LED(AWAKE_LED_PIN);
    static volatile boolean button1Triggered;
    static volatile boolean button2Triggered;
    static volatile boolean button3Triggered;
};

#endif

