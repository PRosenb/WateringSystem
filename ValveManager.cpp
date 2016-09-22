
#include "ValveManager.h"
#include <Time.h>         // http://www.arduino.cc/playground/Code/Time
#include "EEPROMWearLevel.h"

ValveManager::ValveManager(WaterMeter *waterMeter) {
  int durationZone1Sec = DEFAULT_DURATION_AUTOMATIC1_SEC;
  durationZone1Sec = EEPROMwl.get(EEPROM_INDEX_ZONE1, durationZone1Sec);
  int durationZone2Sec = DEFAULT_DURATION_AUTOMATIC2_SEC;
  durationZone2Sec = EEPROMwl.get(EEPROM_INDEX_ZONE2, durationZone2Sec);
  int durationZone3Sec = DEFAULT_DURATION_AUTOMATIC3_SEC;
  durationZone3Sec = EEPROMwl.get(EEPROM_INDEX_ZONE3, durationZone3Sec);

  // set limit
  if (durationZone1Sec > 1000) {
    durationZone1Sec = 1000;
  }
  if (durationZone2Sec > 1000) {
    durationZone2Sec = 1000;
  }
  if (durationZone3Sec > 1000) {
    durationZone3Sec = 1000;
  }

  valveMain = new MeasuredValve(VALVE1_PIN, waterMeter);
  valveArea1 = new Valve(VALVE2_PIN);
  valveArea2 = new Valve(VALVE3_PIN);
  valveArea3 = new Valve(VALVE4_PIN);

  superStateMainIdle = new SuperState("mainIdle");
  superStateMainOn = new ValveSuperState(valveMain, "mainOn");

  stateIdle = new DurationState(0, "areasIdle", superStateMainIdle);
  stateWarnAutomatic1 = new ValveState(valveArea1, (unsigned long) DURATION_WARN_SEC * 1000, "warn", superStateMainOn);
  stateWaitBeforeAutomatic1 = new DurationState((unsigned long) DURATION_WAIT_BEFORE_SEC * 1000, "areasIdle", superStateMainIdle);
  stateAutomatic1 = new ValveState(valveArea1, (unsigned long) durationZone1Sec * 1000, "area1", superStateMainOn);
  // required to switch main valve off in between. Otherwise, the TaskMeter threashold is hit when filling pipe
  stateBeforeWarnAutomatic2 = new DurationState(1000, "beforeWarnArea2", superStateMainIdle);
  stateWarnAutomatic2 = new ValveState(valveArea2, (unsigned long) DURATION_WARN_SEC * 1000, "warnArea2", superStateMainOn);
  stateWaitBeforeAutomatic2 = new DurationState((unsigned long) DURATION_WAIT_BEFORE_SEC * 1000, "areasIdle", superStateMainIdle);
  stateAutomatic2 = new ValveState(valveArea2, (unsigned long) durationZone2Sec * 1000, "area2", superStateMainOn);
  // required to switch main valve off in between. Otherwise, the TaskMeter threashold is hit when filling pipe
  stateBeforeWarnAutomatic3 = new DurationState(1000, "beforeWarnArea3", superStateMainIdle);
  stateWarnAutomatic3 = new ValveState(valveArea3, (unsigned long) DURATION_WARN_SEC * 1000, "warnArea3", superStateMainOn);
  stateWaitBeforeAutomatic3 = new DurationState((unsigned long) DURATION_WAIT_BEFORE_SEC * 1000, "areasIdle", superStateMainIdle);
  stateAutomatic3 = new ValveState(valveArea3, (unsigned long) durationZone3Sec * 1000, "area3", superStateMainOn);
  stateManual = new ValveState(valveArea1, (unsigned long) DURATION_MANUAL_SEC * 1000, "manual", superStateMainOn);

  stateWarnAutomatic1->nextState = stateWaitBeforeAutomatic1;
  stateWaitBeforeAutomatic1->nextState = stateAutomatic1;
  stateAutomatic1->nextState = stateBeforeWarnAutomatic2;

  stateBeforeWarnAutomatic2->nextState = stateWarnAutomatic2;
  stateWarnAutomatic2->nextState = stateWaitBeforeAutomatic2;
  stateWaitBeforeAutomatic2->nextState = stateAutomatic2;
  stateAutomatic2->nextState = stateBeforeWarnAutomatic3;

  stateBeforeWarnAutomatic3->nextState = stateWarnAutomatic3;
  stateWarnAutomatic3->nextState = stateWaitBeforeAutomatic3;
  stateWaitBeforeAutomatic3->nextState = stateAutomatic3;
  stateAutomatic3->nextState = stateIdle;

  stateManual->nextState = stateIdle;

  fsm = new DurationFsm(*stateIdle, "FSM");
}

ValveManager::~ValveManager() {
  delete valveMain;
  delete valveArea1;
  delete valveArea2;
  delete valveArea3;

  delete superStateMainIdle;
  delete superStateMainOn;

  delete stateIdle;
  delete stateWarnAutomatic1;
  delete stateWaitBeforeAutomatic1;
  delete stateAutomatic1;
  delete stateBeforeWarnAutomatic2;
  delete stateWarnAutomatic2;
  delete stateWaitBeforeAutomatic2;
  delete stateAutomatic2;
  delete stateBeforeWarnAutomatic3;
  delete stateWarnAutomatic3;
  delete stateWaitBeforeAutomatic3;
  delete stateAutomatic3;
  delete stateManual;
  delete fsm;
}

void ValveManager::manualMainOn() {
  fsm->changeState(*stateManual);
}

void ValveManager::stopAll() {
  fsm->changeState(*stateIdle);
  // all off, just to be really sure..
  valveMain->off();
  valveArea1->off();
  valveArea2->off();
  valveArea3->off();
}

void ValveManager::startAutomaticWithWarn() {
  // ignore if on manual
  if (!fsm->isInState(*stateManual)) {
    fsm->changeState(*stateWarnAutomatic1);
  }
}

void ValveManager::startAutomatic() {
  if (fsm->isInState(*stateAutomatic1)) {
    fsm->changeState(*stateAutomatic2);
  } else if (fsm->isInState(*stateAutomatic2)) {
    fsm->changeState(*stateAutomatic3);
  } else {
    fsm->changeState(*stateAutomatic1);
  }
}

void ValveManager::setZoneDuration(byte zone, int duration) {
  switch (zone) {
    case 1:
      EEPROMwl.put(EEPROM_INDEX_ZONE1, duration);
      stateAutomatic1->minDurationMs = (unsigned long)duration * 1000;
      break;
    case 2:
      EEPROMwl.put(EEPROM_INDEX_ZONE2, duration);
      stateAutomatic2->minDurationMs = (unsigned long)duration * 1000;
      break;
    case 3:
      EEPROMwl.put(EEPROM_INDEX_ZONE3, duration);
      stateAutomatic3->minDurationMs = (unsigned long)duration * 1000;
      break;
  }
}

void ValveManager::printStatus() {
  Serial.print(F("zone1: "));
  Serial.print(stateAutomatic1->minDurationMs / 1000);
  Serial.print(F("s, zone2: "));
  Serial.print(stateAutomatic2->minDurationMs / 1000);
  Serial.print(F("s, zone3: "));
  Serial.print(stateAutomatic3->minDurationMs / 1000);
  Serial.println(F("s"));

  int value = -1;
  Serial.print(F("eeprom: zone1: "));
  Serial.print(EEPROMwl.get(EEPROM_INDEX_ZONE1, value));
  Serial.print(F(", zone2: "));
  value = -1;
  Serial.print(EEPROMwl.get(EEPROM_INDEX_ZONE2, value));
  Serial.print(F(", zone3: "));
  value = -1;
  Serial.print(EEPROMwl.get(EEPROM_INDEX_ZONE3, value));
  Serial.println();
  EEPROMwl.printStatus();
  EEPROMwl.printBinary(0, 128);
  Serial.println();
}

bool ValveManager::isOn() {
  return !fsm->isInState(*stateIdle);
}

