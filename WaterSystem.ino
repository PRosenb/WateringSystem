
#include <DS3232RTC.h>    // http://github.com/JChristensen/DS3232RTC

#include "SleepManager.h"
#include "WaterManager.h"
#include "WaterMeter.h"

#define STOP_ALL_INT_PIN 0
#define SERIAL_SLEEP_TIMEOUT_MS 30000

void rtcTriggered();
void isrStopAll();

SleepManager *sleepManager;
WaterManager *waterManager;
WaterMeter *waterMeter;
volatile boolean stopAllTriggered = false;
unsigned long serialLastActiveMillis = 0;

void setup() {
  Serial.begin(9600);
  delay(100); // wait for serial to init
  sleepManager = new SleepManager();
  waterManager = new WaterManager();
  waterMeter = new WaterMeter();
  waterMeter->start();

  pinMode(STOP_ALL_INT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(STOP_ALL_INT_PIN), isrStopAll, FALLING);

  RTC.alarmInterrupt(ALARM_1, true);
  RTC.alarmInterrupt(ALARM_2, false);
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

  boolean finished = waterManager->update();
  //    finished = false;
  // do not sleep until Serial is disconnected for SERIAL_SLEEP_TIMEOUT_MS
  if (Serial.available() > 0) {
    serialLastActiveMillis = millis();
  }
  Interrupts interrupts;
  if (finished && millis() - serialLastActiveMillis > SERIAL_SLEEP_TIMEOUT_MS) {
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

  waterMeter->calculate();

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

// sets the system and RTC time.
// used format: 2016-01-03T16:43
void handleSetDateTime() {
  int yearValue = serialReadInt(4);
  char dash1 = Serial.available() ? Serial.read() : 0;
  int monthValue = serialReadInt(2);
  char dash2 = Serial.available() ? Serial.read() : 0;
  int dayValue = serialReadInt(2);
  char tsign = Serial.available() ? Serial.read() : 0;
  int hours = serialReadInt(2);
  char colon = Serial.available() ? Serial.read() : 0;
  int minutes = serialReadInt(2);
  if (colon == ':' && dash1 == '-' && dash2 == '-' && tsign == 'T'
      && monthValue >= 1 && monthValue <= 12
      && dayValue >= 1 && dayValue <= 31
      && hours >= 0 && hours <= 24
      && minutes >= 0 && minutes <= 60) {

    //set the system time (Time lib)
    // setTime(hr,min,sec,day,month,yr)
    setTime(hours, minutes, 0, dayValue, monthValue, yearValue);
    //set the RTC from the system time
    RTC.set(now());

    tmElements_t tm;
    RTC.read(tm);
    Serial.print("RTC values: ");
    Serial.print(tm.Year + 1970, DEC);
    Serial.print('-');
    Serial.print(tm.Month);
    Serial.print('-');
    Serial.print(tm.Day);
    Serial.print(' ');
    Serial.print(tm.Hour);
    Serial.print(':');
    Serial.print(tm.Minute);
    Serial.print(':');
    Serial.println(tm.Second);
  } else {
    Serial.println("Wrong format, e.g. use d2016-01-03T16:43");
  }
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

void handleGetAlarmTime(byte alarmNumber) {
  if (alarmNumber == 0) {
    alarmNumber = 1;
  }
  Serial.print("Alarm");
  Serial.print(alarmNumber);
  Serial.print(": ");
  tmElements_t tm;
  ALARM_TYPES_t alarmType = RTC.readAlarm(alarmNumber, tm);
  Serial.print(tm.Hour);
  Serial.print(":");
  Serial.print(tm.Minute);
  Serial.print(":");
  Serial.print(tm.Second);
  Serial.print(", day: ");
  Serial.print(tm.Day);
  Serial.print(", wday: ");
  Serial.print(tm.Wday);
  Serial.print(", alarmType: ");
  switch (alarmType) {
    case ALM1_EVERY_SECOND:
      Serial.print("ALM1_EVERY_SECOND");
      break;
    case ALM1_MATCH_SECONDS:
      Serial.print("ALM1_MATCH_SECONDS");
      break;
    case ALM1_MATCH_MINUTES:
      Serial.print("ALM1_MATCH_MINUTES");
      break;
    case ALM1_MATCH_HOURS:
      Serial.print("ALM1_MATCH_HOURS");
      break;
    case ALM1_MATCH_DATE:
      Serial.print("ALM1_MATCH_DATE");
      break;
    case ALM1_MATCH_DAY:
      Serial.print("ALM1_MATCH_DAY");
      break;
    case ALM2_EVERY_MINUTE:
      Serial.print("ALM2_EVERY_MINUTE");
      break;
    case ALM2_MATCH_MINUTES:
      Serial.print("ALM2_MATCH_MINUTES");
      break;
    case ALM2_MATCH_HOURS:
      Serial.print("ALM2_MATCH_HOURS");
      break;
    case ALM2_MATCH_DATE:
      Serial.print("ALM2_MATCH_DATE");
      break;
    case ALM2_MATCH_DAY:
      Serial.print("ALM2_MATCH_DAY");
      break;
  }
  Serial.println();
}

void handleSerialInput() {
  char command = Serial.read();
  switch (command) {
    case 'd':
      handleSetDateTime();
      break;
    case 'a':
      handleSetAlarmTime();
      break;
    case 'g':
      handleGetAlarmTime(serialReadInt(1));
      break;
    default:
      Serial.print("Unknown command: ");
      Serial.println(command);
      Serial.println("Supported commands:");
      Serial.println("a<hh>:<mm>: set alarm time");
      Serial.println("g<alarmNumber>: get alarm time");
      Serial.println("d<YYYY>-<MM>-<DD>T<hh>:<mm>: set date/time");
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

