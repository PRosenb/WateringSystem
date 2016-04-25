
#include <DS3232RTC.h>    // http://github.com/JChristensen/DS3232RTC

#define EI_NOTPORTC
#define EI_NOTPORTD
#include <EnableInterrupt.h> // https://github.com/GreyGnome/EnableInterrupt

#include "WaterManager.h"
#include "DeepSleepScheduler.h"
#include "LedState.h"

#define START_SERIAL_PIN 2
// potential PinChangePins on Leonardo: 8, 9, 10, 11
#define RTC_INT_PIN 8
#define START_MANUAL_PIN 9
#define START_AUTOMATIC_PIN 10
#define MODE_PIN 11

#define MODE_COLOR_GREEN_PIN A0
#define MODE_COLOR_RED_PIN A1
#define MODE_COLOR_BLUE_PIN A2

#define SERIAL_SLEEP_TIMEOUT_MS 30000
#define AWAKE_LED_PIN 13

WaterManager *waterManager;
unsigned long serialLastActiveMillis = 0;
boolean aquiredWakeLock = false;

// ModeFsm
DurationState *modeOff;
DurationState *modeAutomatic;
DurationState *modeOffOnce;
DurationFsm *modeFsm;

void setup() {
  int freeRamValue = freeRam();
  Serial.begin(9600);
  delay(100); // wait for serial to init
  Serial.println();
  Serial.print(F("------------------- startup: "));
  Serial.println(freeRamValue);
  delay(100);

  waterManager = new WaterManager();

  RTC.alarmInterrupt(ALARM_1, true);
  RTC.alarmInterrupt(ALARM_2, false);
  // reset alarms if active
  RTC.alarm(ALARM_1);
  RTC.alarm(ALARM_2);
  delay(1000);

  pinMode(START_SERIAL_PIN, INPUT_PULLUP);
  enableInterrupt(START_SERIAL_PIN, isrStartSerial, FALLING);
  pinMode(RTC_INT_PIN, INPUT_PULLUP);
  enableInterrupt(RTC_INT_PIN, isrRtc, FALLING);
  pinMode(START_MANUAL_PIN, INPUT_PULLUP);
  enableInterrupt(START_MANUAL_PIN, isrStartManual, FALLING);
  pinMode(START_AUTOMATIC_PIN, INPUT_PULLUP);
  enableInterrupt(START_AUTOMATIC_PIN, isrStartAutomatic, FALLING);
  pinMode(MODE_PIN, INPUT_PULLUP);
  enableInterrupt(MODE_PIN, isrMode, FALLING);

  pinMode(AWAKE_LED_PIN, OUTPUT);
  scheduler.awakeIndicationPin = AWAKE_LED_PIN;

  initModeFsm();

  startListenToSerial();
}

void loop() {
  scheduler.execute();
}

void initModeFsm() {
  pinMode(MODE_COLOR_GREEN_PIN, OUTPUT);
  pinMode(MODE_COLOR_RED_PIN, OUTPUT);
  pinMode(MODE_COLOR_BLUE_PIN, OUTPUT);
  digitalWrite(MODE_COLOR_GREEN_PIN, HIGH);
  digitalWrite(MODE_COLOR_RED_PIN, HIGH);
  digitalWrite(MODE_COLOR_BLUE_PIN, HIGH);

  modeOff = new ColorLedState(255, MODE_COLOR_RED_PIN, 255, 1, "modeOff");
  modeOffOnce = new ColorLedState(255, MODE_COLOR_RED_PIN, MODE_COLOR_BLUE_PIN, 1, "modeOffOnce");
  modeAutomatic = new ColorLedState(MODE_COLOR_GREEN_PIN, 255, 255, 1, "modeAutomatic");
  modeOff->nextState = modeAutomatic;
  modeAutomatic->nextState = modeOffOnce;
  modeOffOnce->nextState = modeOff;
  modeFsm = DurationFsm::createInstanceWithoutScheduledChangeState(*modeOff, "ModeFSM");
  deactivateModeLed();
}

int freeRam() {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void isrMode() {
  scheduler.schedule(modeScheduled);
}

void modeScheduled() {
  if (waterManager->isOn()) {
    waterManager->stopAll();
  } else {
    // first show current state and then change it
    // LOW == LED active
    if (digitalRead(MODE_COLOR_GREEN_PIN) == LOW
        || digitalRead(MODE_COLOR_RED_PIN) == LOW
        || digitalRead(MODE_COLOR_BLUE_PIN) == LOW) {
      modeFsm->immediatelyChangeToNextState();
    }
  }
  showModeLed();

  // simple debounce
  delay(200);
  scheduler.removeCallbacks(modeScheduled);
}

void deactivateModeLed() {
  digitalWrite(MODE_COLOR_GREEN_PIN, HIGH);
  digitalWrite(MODE_COLOR_RED_PIN, HIGH);
  digitalWrite(MODE_COLOR_BLUE_PIN, HIGH);
}

void startAutomaticRtc() {
  Serial.println(F("startAutomaticRtc")); delay(150);
  if (modeFsm->isInState(*modeAutomatic)) {
    waterManager->startAutomaticWithWarn();
  } else if (modeFsm->isInState(*modeOffOnce)) {
    modeFsm->changeState(*modeAutomatic);
    showModeLed();
  } else {
    showModeLed();
  }
}

void rtcScheduled() {
  //  Serial.println(F("rtcScheduled"));delay(150);
  if (RTC.alarm(ALARM_1)) {
    scheduler.schedule(startAutomaticRtc);
  }
  if (RTC.alarm(ALARM_2)) {
    // not used
  }
}

void isrRtc() {
  scheduler.schedule(rtcScheduled);
}

void startManual() {
  Serial.println(F("startManual"));
  waterManager->manualMainOn();
}

void isrStartManual() {
  scheduler.schedule(startManual);
}

void startAutomatic() {
  Serial.println(F("startAutomatic"));
  waterManager->startAutomatic();
}

void isrStartAutomatic() {
  scheduler.schedule(startAutomatic);
}

void isrStartSerial() {
  scheduler.schedule(startListenToSerial);
}

void showModeLed() {
  ((ColorLedState&)modeFsm->getCurrentState()).reactivateLed();
  scheduler.removeCallbacks(deactivateModeLed);
  scheduler.scheduleDelayed(deactivateModeLed, 10000);
}

void printTime() {
  setSyncProvider(RTC.get);
  Serial.print(hour(), DEC);
  Serial.print(F(":"));
  Serial.print(minute(), DEC);
  Serial.print(F(":"));
  Serial.print(second(), DEC);
  Serial.print(F(" "));
  Serial.println(freeRam());
  delay(100);
}

// TODO restart on serial connect
void startListenToSerial() {
  Serial.println(F("startListenToSerial"));
  serialLastActiveMillis = millis();
  scheduler.removeCallbacks(listenToSerial);
  if (!aquiredWakeLock) {
    aquiredWakeLock = true;
    scheduler.aquireNoDeepSleepLock();
  }
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
    if (aquiredWakeLock) {
      aquiredWakeLock = false;
      scheduler.releaseNoDeepSleepLock();
      Serial.println(F("stop serial"));
    }
  } else {
    scheduler.scheduleDelayed(listenToSerial, 1000);
  }
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

void handleSetAlarmTime() {
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

void handleGetAlarmTime(byte alarmNumber) {
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

