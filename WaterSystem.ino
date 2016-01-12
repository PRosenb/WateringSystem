
#include <DS3232RTC.h>    // http://github.com/JChristensen/DS3232RTC
#include <PinChangeInt.h> // https://github.com/GreyGnome/PinChangeInt

#include "WaterManager.h"
#include "WaterMeter.h"
#include "DeepSleepScheduler.h"

#define START_SERIAL_PIN 2
// potential PinChangePins on Leonardo: 8, 9, 10, 11
#define RTC_INT_PIN 8
#define START_MANUAL_PIN 9
#define START_AUTOMATIC_PIN 10
#define STOP_ALL_PIN 11

#define SERIAL_SLEEP_TIMEOUT_MS 30000
#define AWAKE_LED_PIN 13

WaterManager *waterManager;
WaterMeter *waterMeter;
unsigned long serialLastActiveMillis = 0;

void setup() {
  Serial.begin(9600);
  delay(100); // wait for serial to init
  Serial.println("------------------- startup");
  delay(100);
  waterManager = new WaterManager();
  waterMeter = new WaterMeter();
  waterMeter->start();

  RTC.alarmInterrupt(ALARM_1, true);
  RTC.alarmInterrupt(ALARM_2, false);
  // reset alarms if active
  RTC.alarm(ALARM_1);
  RTC.alarm(ALARM_2);

  pinMode(START_SERIAL_PIN, INPUT_PULLUP);
  attachPinChangeInterrupt(START_SERIAL_PIN, isrStartSerial, FALLING);
  pinMode(RTC_INT_PIN, INPUT_PULLUP);
  attachPinChangeInterrupt(RTC_INT_PIN, isrRtc, FALLING);
  pinMode(START_MANUAL_PIN, INPUT_PULLUP);
  attachPinChangeInterrupt(START_MANUAL_PIN, isrStartManual, FALLING);
  pinMode(START_AUTOMATIC_PIN, INPUT_PULLUP);
  attachPinChangeInterrupt(START_AUTOMATIC_PIN, isrStartAutomatic, FALLING);
  pinMode(STOP_ALL_PIN, INPUT_PULLUP);
  attachPinChangeInterrupt(STOP_ALL_PIN, isrStopAll, FALLING);

  pinMode(AWAKE_LED_PIN, OUTPUT);
  scheduler.awakeIndicationPin = AWAKE_LED_PIN;

  startListenToSerial();
}

void loop() {
  scheduler.execute();
}

void startAutomaticRtc() {
  Serial.println("startAutomaticRtc");
  waterManager->startAutomaticWithWarn();
}

void isrRtc() {
  if (RTC.alarm(ALARM_1)) {
    scheduler.schedule(startAutomaticRtc);
  }
  if (RTC.alarm(ALARM_2)) {
    // not used
  }
}

void startManual() {
  Serial.println("startManual");
  waterManager->manualMainOn();
}

void isrStartManual() {
  scheduler.schedule(startManual);
}

void startAutomatic() {
  Serial.println("startAutomatic");
  waterManager->startAutomatic();
}

void isrStartAutomatic() {
  scheduler.schedule(startAutomatic);
}

void stopAll() {
  Serial.println("stopAll");
  waterManager->stopAll();
}

void isrStopAll() {
  scheduler.schedule(stopAll);
}

void isrStartSerial() {
  scheduler.schedule(startListenToSerial);
}

void printTime() {
  setSyncProvider(RTC.get);
  Serial.print(hour(), DEC);
  Serial.print(':');
  Serial.print(minute(), DEC);
  Serial.print(':');
  Serial.println(second(), DEC);
  delay(100);
}

void startListenToSerial() {
  serialLastActiveMillis = millis();
  scheduler.removeCallbacks(listenToSerial);
  delay(200);
  listenToSerial();
}

void listenToSerial() {
  printTime();
  if (Serial.available() > 0) {
    serialLastActiveMillis = millis();
    handleSerialInput();
  }
  if (millis() - serialLastActiveMillis > SERIAL_SLEEP_TIMEOUT_MS) {
    scheduler.deepSleep = true;
  } else {
    scheduler.scheduleDelayed(listenToSerial, 1000);
    scheduler.deepSleep = false;
  }
  delay(1000);
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
    delay(1000);

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

