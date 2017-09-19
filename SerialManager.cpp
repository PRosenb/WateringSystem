
#include "SerialManager.h"
#include <DS3232RTC.h>    // http://github.com/JChristensen/DS3232RTC
#include <EEPROMWearLevel.h> // https://github.com/PRosenb/EEPROMWearLevel

SerialManager::SerialManager(byte bluetoothEnablePin) :  bluetoothEnablePin(bluetoothEnablePin) {
  if (bluetoothEnablePin != UNDEFINED) {
    pinMode(bluetoothEnablePin, OUTPUT);
  }
  int freeRamValue = freeRam();
  Serial.begin(9600);
  // wait for serial port to connect
  while (!Serial);

  // print startup message
  Serial.println();
  Serial.print(F("------------------- startup, free RAM: "));
  Serial.println(freeRamValue);
  setSyncProvider(RTC.get);
  startupTime = now();
  if (timeStatus() != timeSet) {
    Serial.println(F("RTC: Unable to sync"));
  } else {
    Serial.println(F("RTC: Set the system time"));
  }
  printTime(startupTime);
}

void SerialManager::setWaterManager(WaterManager *waterManager) {
  SerialManager::waterManager = waterManager;
}

void SerialManager::startSerial(unsigned long durationMs) {
  SerialManager::durationMs = durationMs;
  if (bluetoothEnablePin != UNDEFINED) {
    digitalWrite(bluetoothEnablePin, HIGH);
  }

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
  if (Serial.available() > 0) {
    serialLastActiveMillis = millis();
    handleSerialInput();
  }

  if (millis() - serialLastActiveMillis > durationMs) {
    if (aquiredWakeLock) {
      aquiredWakeLock = false;
      scheduler.releaseNoDeepSleepLock();
      Serial.println(F("stop serial"));
      delay(150);
      if (bluetoothEnablePin != UNDEFINED) {
        digitalWrite(bluetoothEnablePin, LOW);
      }
    }
  } else {
    scheduler.scheduleDelayed(this, 1000);
  }
}

void SerialManager::printTime(time_t t) {
  Serial.print(year(t));
  Serial.print(F("-"));
  printTwoDigits(month(t));
  Serial.print(F("-"));
  printTwoDigits(day(t));
  Serial.print(F(" "));
  printTwoDigits(hour(t));
  Serial.print(F(":"));
  printTwoDigits(minute(t));
  Serial.print(F(":"));
  printTwoDigits(second(t));
  Serial.print(F(", free RAM: "));
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
  if (colon == ':' && dash1 == '-' && dash2 == '-' && (tsign == 'T' || tsign == ' ')
      && monthValue >= 1 && monthValue <= 12
      && dayValue >= 1 && dayValue <= 31
      && hours >= 0 && hours <= 24
      && minutes >= 0 && minutes <= 60) {

    //set the system time (Time lib)
    // setTime(hr,min,sec,day,month,yr)
    setTime(hours, minutes, 0, dayValue, monthValue, yearValue);
    //set the RTC from the system time
    RTC.set(now());
  } else {
    Serial.println(F("Wrong format, e.g. use d2016-01-03T16:43"));
  }
  tmElements_t tm;
  RTC.read(tm);
  Serial.print(F("RTC values: "));
  Serial.print(tm.Year + 1970, DEC);
  Serial.print(F("-"));
  printTwoDigits(tm.Month);
  Serial.print(F("-"));
  printTwoDigits(tm.Day);
  Serial.print(F(" "));
  printTwoDigits(tm.Hour);
  Serial.print(F(":"));
  printTwoDigits(tm.Minute);
  Serial.print(F(":"));
  printTwoDigits(tm.Second);
  Serial.println();
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
  printTwoDigits(tm.Hour);
  Serial.print(F(":"));
  printTwoDigits(tm.Minute);
  Serial.print(F(":"));
  printTwoDigits(tm.Second);
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
  printTwoDigits(tm.Hour);
  Serial.print(F(":"));
  printTwoDigits(tm.Minute);
  Serial.println();
}

void SerialManager::handleStatus() {
  const byte subCommand = Serial.available() ? Serial.read() : 0;
  if (subCommand == 'e') {
    char colon = Serial.available() ? Serial.read() : 0;
    int startAddress = serialReadInt(3);
    char comma = Serial.available() ? Serial.read() : 0;
    int endAddress = serialReadInt(3);
    EEPROMwl.printStatus(Serial);
    if (endAddress > 0) {
      EEPROMwl.printBinary(Serial, startAddress, endAddress);
      Serial.println();
    }
  } else {
    Serial.print(F("Startup time: "));
    printTime(startupTime);
    Serial.print(F("Current time: "));
    setSyncProvider(RTC.get);
    printTime(now());
    int resetCount = 0;
    EEPROMwl.get(EEPROM_INDEX_WATCHDOG_RESET_COUNT, resetCount);
    Serial.print(F("WD reset count: "));
    Serial.println(resetCount);
    waterManager->printStatus();
  }
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
    case 't':
      RTC.setAlarm(ALM1_MATCH_SECONDS, 0, 0, 0, 0);
      Serial.println(F("alarm every minute"));
      break;
    case 'g':
      handleGetAlarmTime(serialReadInt(1));
      break;
    case 'm':
      waterManager->modeClicked();
      break;
    case 'i':
      waterManager->startAutomatic();
      break;
    case 'j':
      waterManager->startAutomaticRtc();
      break;
    case 's':
      handleStatus();
      break;
    case 'w':
      handleWrite();
      break;
    default:
      Serial.print(F("Unknown command: "));
      Serial.println(command);
      Serial.println(F("Supported commands:"));
      Serial.println(F("a<hh>:<mm>: set alarm time"));
      Serial.println(F("t set alarm every minute (for testing)"));
      Serial.println(F("g<alarmNumber>: get alarm time"));
      Serial.println(F("d<YYYY>-<MM>-<DD>T<hh>:<mm>: set date/time"));
      Serial.println(F("m: change mode"));
      Serial.println(F("i: start automatic"));
      Serial.println(F("j: start automatic RTC"));
      Serial.println(F("wz<zone>:<value 3 digits> write zone duration in minutes"));
      Serial.println(F("wm:<value 3 digits> write water meter stop threshold"));
      Serial.println(F("s print status"));
      Serial.println(F("se print status of EEPROM"));
      Serial.println(F("se:<from 3 digits>,<to 3 digits> print status of EEPROM"));
  }
}

void SerialManager::handleWrite() {
  char writeType = Serial.read();
  switch (writeType) {
    case 'z': {
        int zoneNr = serialReadInt(1);
        Serial.read(); // the :
        int durationMin = serialReadInt(3);
        Serial.print(F("handleWrite: "));
        Serial.print(writeType);
        Serial.print(F(" "));
        Serial.print(zoneNr);
        Serial.print(F(" "));
        Serial.println(durationMin);
        unsigned int durationSec = durationMin * 60UL;
        waterManager->setZoneDuration(zoneNr, durationSec);
        break;
      }
    case 'm':
      Serial.read(); // the :
      int waterMeterStopThreshold = serialReadInt(3);
      Serial.print(F("waterMeterStopThreshold: "));
      Serial.println(waterMeterStopThreshold);
      waterManager->setWaterMeterStopThreshold(waterMeterStopThreshold);
      break;
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

void SerialManager::printTwoDigits(unsigned int value) {
  if (value < 10) {
    Serial.print(F("0"));
  }
  Serial.print(value);
}

int SerialManager::freeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
