
#include "ValveManager.h"
#include <Time.h>         // http://www.arduino.cc/playground/Code/Time
#include <EEPROMWearLevel.h> // https://github.com/PRosenb/EEPROMWearLevel

ValveManager::ValveManager(WaterMeter *waterMeter, const Runnable * const leakCheckListener) {
  unsigned int durationZone1Sec = DEFAULT_DURATION_AUTOMATIC1_SEC;
  durationZone1Sec = EEPROMwl.get(EEPROM_INDEX_ZONE1, durationZone1Sec);
  unsigned int durationZone2Sec = DEFAULT_DURATION_AUTOMATIC2_SEC;
  durationZone2Sec = EEPROMwl.get(EEPROM_INDEX_ZONE2, durationZone2Sec);
  unsigned int durationZone3Sec = DEFAULT_DURATION_AUTOMATIC3_SEC;
  durationZone3Sec = EEPROMwl.get(EEPROM_INDEX_ZONE3, durationZone3Sec);

  // set limit
  if (durationZone1Sec > MAX_ZONE_DURATION) {
    durationZone1Sec = MAX_ZONE_DURATION;
  }
  if (durationZone2Sec > MAX_ZONE_DURATION) {
    durationZone2Sec = MAX_ZONE_DURATION;
  }
  if (durationZone3Sec > MAX_ZONE_DURATION) {
    durationZone3Sec = MAX_ZONE_DURATION;
  }

  valveMain = new MeasuredValve(VALVE1_PIN, waterMeter);
  valveArea1 = new Valve(VALVE2_PIN);
  valveArea2 = new Valve(VALVE3_PIN);
  valveArea3 = new Valve(VALVE4_PIN);

  superStateMainIdle = new SuperState(F("mainIdle"));
  superStateMainOn = new ValveSuperState(valveMain, F("mainOn"));

  class SensorCheckListener: public MeasureStateListener {
      virtual void measuredResult(unsigned int tickCount) {
        Serial.print(F("SensorCheckListener: "));
        Serial.println(tickCount);
      }
  };
  const MeasureStateListener * const sensorCheckListener = new SensorCheckListener();

  stateIdle = new DurationState(0, F("idle"), superStateMainIdle);
  stateLeakCheckFill = new DurationState(DURATION_LEAK_CHECK_FILL_MS, F("leakCheckFill"), superStateMainOn);
  stateLeakCheckWait = new LeakCheckState(DURATION_LEAK_CHECK_WAIT_MS, F("leakCheckWait"), superStateMainOn, leakCheckListener, waterMeter);
  stateWarnAutomatic1 = new MeasureState(valveArea1, DURATION_WARN_SEC * 1000UL, F("warnArea1"), superStateMainOn, sensorCheckListener, waterMeter);
  stateWaitBeforeAutomatic1 = new DurationState(DURATION_WAIT_BEFORE_SEC * 1000UL, F("idleArea1"), superStateMainIdle);
  stateAutomatic1 = new ValveState(valveArea1, durationZone1Sec * 1000UL, F("area1"), superStateMainOn);
  // required to switch main valve off in between. Otherwise, the TaskMeter threashold is hit when filling pipe
  stateBeforeWarnAutomatic2 = new DurationState(1000UL, F("beforeWarnArea2"), superStateMainIdle);
  stateWarnAutomatic2 = new ValveState(valveArea2, DURATION_WARN_SEC * 1000UL, F("warnArea2"), superStateMainOn);
  stateWaitBeforeAutomatic2 = new DurationState(DURATION_WAIT_BEFORE_SEC * 1000UL, F("idleArea2"), superStateMainIdle);
  stateAutomatic2 = new ValveState(valveArea2, durationZone2Sec * 1000UL, F("area2"), superStateMainOn);
  // required to switch main valve off in between. Otherwise, the TaskMeter threashold is hit when filling pipe
  stateBeforeWarnAutomatic3 = new DurationState(1000UL, F("beforeWarnArea3"), superStateMainIdle);
  stateWarnAutomatic3 = new ValveState(valveArea3, DURATION_WARN_SEC * 1000UL, F("warnArea3"), superStateMainOn);
  stateWaitBeforeAutomatic3 = new DurationState(DURATION_WAIT_BEFORE_SEC * 1000UL, F("idleArea3"), superStateMainIdle);
  stateAutomatic3 = new ValveState(valveArea3, durationZone3Sec * 1000UL, F("area3"), superStateMainOn);
  stateManual = new ValveState(valveArea1, DURATION_MANUAL_SEC * 1000UL, F("manual"), superStateMainOn);

  stateLeakCheckFill->nextState = stateLeakCheckWait;
  stateLeakCheckWait->nextState = stateWarnAutomatic1;
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

  fsm = new DurationFsm(*stateIdle, F("FSM"));
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

void ValveManager::startManual() {
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
#ifdef LEAK_CHECK
    fsm->changeState(*stateLeakCheckFill);
#else
    fsm->changeState(*stateWarnAutomatic1);
#endif
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

void ValveManager::setZoneDuration(byte zone, unsigned int durationSec) {
  switch (zone) {
    case 1:
      EEPROMwl.put(EEPROM_INDEX_ZONE1, durationSec);
      stateAutomatic1->minDurationMs = durationSec * 1000UL;
      break;
    case 2:
      EEPROMwl.put(EEPROM_INDEX_ZONE2, durationSec);
      stateAutomatic2->minDurationMs = durationSec * 1000UL;
      break;
    case 3:
      EEPROMwl.put(EEPROM_INDEX_ZONE3, durationSec);
      stateAutomatic3->minDurationMs = durationSec * 1000UL;
      break;
  }
}

void ValveManager::printStatus() {
  Serial.print(F("zone1: "));
  Serial.print(stateAutomatic1->minDurationMs / 1000UL / 60UL);
  Serial.print(F(" min, zone2: "));
  Serial.print(stateAutomatic2->minDurationMs / 1000UL / 60UL);
  Serial.print(F(" min, zone3: "));
  Serial.print(stateAutomatic3->minDurationMs / 1000UL / 60UL);
  Serial.println(F(" min"));

  unsigned int value = -1;
  Serial.print(F("eeprom: zone1: "));
  Serial.print(EEPROMwl.get(EEPROM_INDEX_ZONE1, value));
  Serial.print(F(", zone2: "));
  value = -1;
  Serial.print(EEPROMwl.get(EEPROM_INDEX_ZONE2, value));
  Serial.print(F(", zone3: "));
  value = -1;
  Serial.print(EEPROMwl.get(EEPROM_INDEX_ZONE3, value));
  Serial.println();
}

bool ValveManager::isOn() {
  return !fsm->isInState(*stateIdle);
}

