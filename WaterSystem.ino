
#include <DS3232RTC.h>    //http://github.com/JChristensen/DS3232RTC

#include "SleepManager.h"
#include "WaterManager.h"

SleepManager *sleepManager;
WaterManager *waterManager;

void setup() {
  Serial.begin(9600);
  delay(100); // every second
  sleepManager = new SleepManager();
  waterManager = new WaterManager();

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
  //  finished = false;
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
    waterManager->stopAll();
  }
  if (interrupts.button2) {
    Serial.println("BUTTON_2_INT");
    waterManager->manualMainOn();
  }
  if (interrupts.button3) {
    Serial.println("BUTTON_3_INT");
  }

  // allows also to sync time after wakeup
  delay(1000); // every second
}

void rtcTriggered() {
  Serial.println("alarm triggered");
  int t = RTC.temperature();
  float celsius = t / 4.0;
  Serial.print("temp:");
  Serial.println(celsius, DEC);
}

