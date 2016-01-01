
#include <DS3232RTC.h>    //http://github.com/JChristensen/DS3232RTC

#include "SleepManager.h"
#include "WaterManager.h"

#define STOP_ALL_INT_PIN 0

void rtcTriggered();
void isrStopAll();

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

  //  RTC.setAlarm(ALM1_MATCH_SECONDS, 0, 0, 0, 0);
  //  RTC.alarmInterrupt(ALARM_1, true);
  //  RTC.alarmInterrupt(ALARM_2, false);

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

  if (Serial.available() > 0) {
    handleSerialInput();
  }

  Interrupts interrupts;
  boolean finished = waterManager->update();
  //    finished = false;
  // do not sleep when Serial is connected and within the first 30 sec
  if (finished && !Serial && millis() > 30000) {
    interrupts = sleepManager->sleep();
  } else {
    interrupts = sleepManager->getSeenInterruptsAndClear();
  }

  if (interrupts.rtcAlarm1 > 0) {
    Serial.println("RTC1_INT");
    rtcTriggered();
  }
  if (interrupts.rtcAlarm2 > 0) {
    Serial.println("RTC2_INT");
  }
  if (interrupts.wakeupInterrupt1 > 0) {
    Serial.print("WAKEUP_INTERRUPT_1: ");
    Serial.println(interrupts.wakeupInterrupt1, DEC);
    waterManager->manualMainOn();
  }
  if (interrupts.wakeupInterrupt2 > 0) {
    Serial.print("WAKEUP_INTERRUPT_2: ");
    Serial.println(interrupts.wakeupInterrupt2, DEC);
    waterManager->startAutomatic();
  }
  if (interrupts.wakeupInterrupt3 > 0) {
    Serial.print("WAKEUP_INTERRUPT_3: ");
    Serial.println(interrupts.wakeupInterrupt3, DEC);
  }
  if (stopAllTriggered) {
    stopAllTriggered = false;
    waterManager->stopAll();
  }

  // allows also to sync time after wakeup
  delay(1000); // repeat every second
}

void readSerial(char inData[], int inDataLength) {
  byte index = 0;
  // index + 1 to allow one space for the Null termination
  for (; Serial.available() > 0 && index + 1 < inDataLength; index++) {
    inData[index] = Serial.read();
  }
  inData[index] = '\0'; // Null terminate the string
}

int serialReadInt(int length) {
  char inData[length + 1];
  readSerial(inData, length + 1);
  return atoi(inData);
}

void handleSetAlarmTime() {
  int hours = serialReadInt(2);
  char colon = Serial.available() ? Serial.read() : 0;
  int minutes = serialReadInt(2);
  if (colon == ':' && hours >= 0 && hours <= 24 && minutes >= 0 && minutes <= 60) {
    //setAlarm(ALARM_TYPES_t alarmType, byte seconds, byte minutes, byte hours, byte daydate);
    RTC.setAlarm(ALM1_MATCH_HOURS, 0, minutes, hours, 0);
    
    Serial.print("set start time to ");
    Serial.print(hours);
    Serial.print(":");
    Serial.println(minutes);
  } else {
    Serial.println("set start time failed, wrong format, expect s<hh>:<mm>, e.g. s14:45");
  }
}

void handleSerialInput() {
  char command = Serial.read();
  switch (command) {
    case 's':
      handleSetAlarmTime();
      break;
    default:
      Serial.print("Unknown command: ");
      Serial.println(command);
      Serial.println("supported commands:");
      Serial.println("s<hh>:<mm>: set alarm time");
  }
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

