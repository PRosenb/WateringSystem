
#include "SerialManager.h"
#include <DS3232RTC.h>    // http://github.com/JChristensen/DS3232RTC

SerialManager::SerialManager() {
  int freeRamValue = freeRam();
  Serial.begin(9600);
  delay(100); // wait for serial to init
  Serial.println();
  Serial.print(F("------------------- startup: "));
  Serial.println(freeRamValue);
}

void SerialManager::startSerial(unsigned int durationMs) {
  SerialManager::durationMs = durationMs;

  serialLastActiveMillis = millis();
  if (!aquiredWakeLock) {
    aquiredWakeLock = true;
    scheduler.acquireNoDeepSleepLock();
  }

  if (!scheduler.isScheduled(this) ) {
    scheduler.schedule(this);
  }
}

void SerialManager::run() {
  printTime();
  if (Serial.available() > 0) {
    serialLastActiveMillis = millis();
    handleSerialInput();
  }
  //  Serial.println(F("1"));
  if (millis() - serialLastActiveMillis > durationMs) {
    if (aquiredWakeLock) {
      aquiredWakeLock = false;
      scheduler.releaseNoDeepSleepLock();
      Serial.println(F("stop serial"));
    }
  } else {
    scheduler.scheduleDelayed(this, 1000);
  }
}

void SerialManager::printTime() {
  setSyncProvider(RTC.get);
  Serial.print(hour(), DEC);
  Serial.print(F(":"));
  Serial.print(minute(), DEC);
  Serial.print(F(":"));
  Serial.print(second(), DEC);
  Serial.print(F(" "));
  //  Serial.print(waterManager->getUsedWater());
  Serial.print(F(" "));
  Serial.println(freeRam());
}

// sets the system and RTC time.
// used format: 2016-01-03T16:43
void SerialManager::handleSetDateTime() {
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
    Serial.print(F("RTC values: "));
    Serial.print(tm.Year + 1970, DEC);
    Serial.print(F("-"));
    Serial.print(tm.Month);
    Serial.print(F("-"));
    Serial.print(tm.Day);
    Serial.print(F(" "));
    Serial.print(tm.Hour);
    Serial.print(F(":"));
    Serial.print(tm.Minute);
    Serial.print(F(":"));
    Serial.println(tm.Second);
  } else {
    Serial.println(F("Wrong format, e.g. use d2016-01-03T16:43"));
  }
}

void SerialManager::handleSetAlarmTime() {
  int hours = serialReadInt(2);
  char colon = Serial.available() ? Serial.read() : 0;
  int minutes = serialReadInt(2);
  if (colon == ':' && hours >= 0 && hours <= 24 && minutes >= 0 && minutes <= 60) {
    //setAlarm(ALARM_TYPES_t alarmType, byte seconds, byte minutes, byte hours, byte daydate);
    RTC.setAlarm(ALM1_MATCH_HOURS, 0, minutes, hours, 0);

    Serial.print(F("set start time to "));
    Serial.print(hours);
    Serial.print(F(":"));
    Serial.println(minutes);
  } else {
    Serial.println(F("set start time failed, wrong format, expect s<hh>:<mm>, e.g. a14:45"));
  }
}

void SerialManager::handleGetAlarmTime(byte alarmNumber) {
  if (alarmNumber == 0) {
    alarmNumber = 1;
  }
  Serial.print(F("Alarm"));
  Serial.print(alarmNumber);
  Serial.print(F(": "));
  tmElements_t tm;
  ALARM_TYPES_t alarmType = RTC.readAlarm(alarmNumber, tm);
  Serial.print(tm.Hour);
  Serial.print(F(":"));
  Serial.print(tm.Minute);
  Serial.print(F(":"));
  Serial.print(tm.Second);
  Serial.print(F(", day: "));
  Serial.print(tm.Day);
  Serial.print(F(", wday: "));
  Serial.print(tm.Wday);
  Serial.print(F(", alarmType: "));
  switch (alarmType) {
    case ALM1_EVERY_SECOND:
      Serial.print(F("ALM1_EVERY_SECOND"));
      break;
    case ALM1_MATCH_SECONDS:
      Serial.print(F("ALM1_MATCH_SECONDS"));
      break;
    case ALM1_MATCH_MINUTES:
      Serial.print(F("ALM1_MATCH_MINUTES"));
      break;
    case ALM1_MATCH_HOURS:
      Serial.print(F("ALM1_MATCH_HOURS"));
      break;
    case ALM1_MATCH_DATE:
      Serial.print(F("ALM1_MATCH_DATE"));
      break;
    case ALM1_MATCH_DAY:
      Serial.print(F("ALM1_MATCH_DAY"));
      break;
    case ALM2_EVERY_MINUTE:
      Serial.print(F("ALM2_EVERY_MINUTE"));
      break;
    case ALM2_MATCH_MINUTES:
      Serial.print(F("ALM2_MATCH_MINUTES"));
      break;
    case ALM2_MATCH_HOURS:
      Serial.print(F("ALM2_MATCH_HOURS"));
      break;
    case ALM2_MATCH_DATE:
      Serial.print(F("ALM2_MATCH_DATE"));
      break;
    case ALM2_MATCH_DAY:
      Serial.print(F("ALM2_MATCH_DAY"));
      break;
  }
  Serial.print(F(", cmd: a"));
  if (tm.Hour < 10) {
    Serial.print(F("0"));
  }
  Serial.print(tm.Hour);
  Serial.print(F(":"));
  if (tm.Minute < 10) {
    Serial.print(F("0"));
  }
  Serial.print(tm.Minute);
  Serial.println();
}

void SerialManager::handleSerialInput() {
  char command = Serial.read();
  switch (command) {
    case 'd':
      handleSetDateTime();
      break;
    case 'a':
      handleSetAlarmTime();
      break;
    case 'm':
      RTC.setAlarm(ALM1_MATCH_SECONDS, 0, 0, 0, 0);
      Serial.println(F("alarm every minute"));
      break;
    case 'g':
      handleGetAlarmTime(serialReadInt(1));
      break;
    default:
      Serial.print(F("Unknown command: "));
      Serial.println(command);
      Serial.println(F("Supported commands:"));
      Serial.println(F("a<hh>:<mm>: set alarm time"));
      Serial.println(F("m set alarm every minute (for testing)"));
      Serial.println(F("g<alarmNumber>: get alarm time"));
      Serial.println(F("d<YYYY>-<MM>-<DD>T<hh>:<mm>: set date/time"));
  }
}

// ----------------------------------------------------------------------------------
// helper
// ----------------------------------------------------------------------------------
void SerialManager::readSerial(char inData[], int inDataLength) {
  byte index = 0;
  // index + 1 to allow one space for the Null termination
  for (; Serial.available() > 0 && index + 1 < inDataLength; index++) {
    inData[index] = Serial.read();
  }
  inData[index] = '\0'; // Null terminate the string
}

int SerialManager::serialReadInt(int length) {
  char inData[length + 1];
  readSerial(inData, length + 1);
  return atoi(inData);
}

int SerialManager::freeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
