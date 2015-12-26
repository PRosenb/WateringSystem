
#include <DS3232RTC.h>    //http://github.com/JChristensen/DS3232RTC

#include "SleepManager.h"
#include "WaterManager.h"

#define STOP_ALL_INT_PIN 0

SleepManager *sleepManager;
WaterManager *waterManager;
volatile boolean stopAllTriggered = false;

void setup() {
  Serial.begin(9600);
  delay(100); // wait for serial to init
  sleepManager = new SleepManager();
  waterManager = new WaterManager();

  pinMode(STOP_ALL_INT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(STOP_ALL_INT_PIN), isrStopAll, FALLING);

  //  RTC.setAlarm(ALM1_MATCH_SECONDS, 0, 0, 0, 1);
  //  RTC.alarmInterrupt(ALARM_1, true);

  //this example first sets the system time (maintained by the Time library) to
  //a hard-coded date and time, and then sets the RTC from the system time.
  //the setTime() function is part of the Time library.
  //setTime(hr,min,sec,day,month,yr)
  //  setTime(22, 23, 0, 16, 1, 2015);   //set the system time
  //  RTC.set(now());                     //set the RTC from the system time
}

void loop() {
  Serial.print(hour(), DEC);
  Serial.print(':');
  Serial.print(minute(), DEC);
  Serial.print(':');
  Serial.println(second(), DEC);

  Interrupts interrupts;
  boolean finished = waterManager->update();
//    finished = false;
  if (finished) {
    interrupts = sleepManager->sleep();
  } else {
    interrupts = sleepManager->getSeenInterruptsAndClear();
  }

  if (interrupts.rtcAlarm1 || interrupts.rtcAlarm2) {
    Serial.println("RTC_INT");
    rtcTriggered();
  }
  if (interrupts.button1) {
    Serial.println("BUTTON_1_INT");
    waterManager->manualMainOn();
  }
  if (interrupts.button2) {
    Serial.println("BUTTON_2_INT");
    waterManager->startAutomatic();
  }
  if (interrupts.button3) {
    Serial.println("BUTTON_3_INT");
  }
  if (stopAllTriggered) {
    stopAllTriggered = false;
    waterManager->stopAll();    
  }

  // allows also to sync time after wakeup
  delay(1000); // repeat every second
}

void rtcTriggered() {
  Serial.println("alarm triggered");
  int t = RTC.temperature();
  float celsius = t / 4.0;
  Serial.print("temp:");
  Serial.println(celsius, DEC);
  waterManager->startAutomaticWithWarn();
}

void isrStopAll() {
  stopAllTriggered = true;
}

