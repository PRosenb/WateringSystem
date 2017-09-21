
#include "WaterManager.h"
#include "LedState.h"
#include <EEPROMWearLevel.h> // https://github.com/PRosenb/EEPROMWearLevel

WaterManager::WaterManager() {
  unsigned int waterMeterStopThreshold = DEFAULT_WATER_METER_STOP_THRESHOLD;
  waterMeterStopThreshold = EEPROMwl.get(EEPROM_INDEX_WATER_METER_THRESHOLD, waterMeterStopThreshold);
  stoppedByThreshold = 0;

  waterMeter = new WaterMeter(1000);
  waterMeter->setThresholdSupervisionDelay(PIPE_FILLING_TIME_MS);
  waterMeter->setThresholdListener(waterMeterStopThreshold, this);

  valveManager = new ValveManager(waterMeter, leakCheckListener);

  initModeFsm();
}

WaterManager::~WaterManager() {
  delete valveManager;
  delete waterMeter;

  delete modeOff;
  delete modeOffOnce;
  delete modeAutomatic;
  delete modeFsm;
}

void WaterManager::run() {
  stoppedByThreshold = waterMeter->getLastPulseCountOverThreshold();
  Serial.print(F("ThresholdListener: "));
  Serial.println(stoppedByThreshold);
  valveManager->stopAll();
  modeFsm->changeState(*modeOff);
}

void WaterManager::leakCheckListenerCallback() {
  Serial.println("Leak detected");
  valveManager->stopAll();
  modeFsm->changeState(*modeOff);
}

void WaterManager::initModeFsm() {
  pinMode(MODE_COLOR_GREEN_PIN, OUTPUT);
  pinMode(MODE_COLOR_RED_PIN, OUTPUT);
  pinMode(MODE_COLOR_BLUE_PIN, OUTPUT);
  digitalWrite(MODE_COLOR_GREEN_PIN, HIGH);
  digitalWrite(MODE_COLOR_RED_PIN, HIGH);
  digitalWrite(MODE_COLOR_BLUE_PIN, HIGH);

  modeOff = new ColorLedState(255, MODE_COLOR_RED_PIN, 255, 10000, F("modeOff"));
  modeOffOnce = new ColorLedState(255, MODE_COLOR_RED_PIN, MODE_COLOR_BLUE_PIN, 10000, F("modeOffOnce"));
  modeAutomatic = new ColorLedState(MODE_COLOR_GREEN_PIN, 255, 255, 10000, F("modeAutomatic"));
  // for simple LED test:
  //  modeOff = new ColorLedState(255, MODE_COLOR_RED_PIN, MODE_COLOR_BLUE_PIN, 10000, F("modeOff"));
  //  modeOffOnce = new ColorLedState(MODE_COLOR_GREEN_PIN, 255, MODE_COLOR_BLUE_PIN, 10000, F("modeOffOnce"));
  //  modeAutomatic = new ColorLedState(MODE_COLOR_GREEN_PIN, MODE_COLOR_RED_PIN, 255, 10000, F("modeAutomatic"));
  modeOff->nextState = modeAutomatic;
  modeAutomatic->nextState = modeOffOnce;
  modeOffOnce->nextState = modeOff;

  modeFsm = new DurationFsm(*modeOff, F("ModeFSM"));
}

void WaterManager::setZoneDuration(byte zone, unsigned int durationSec) {
  valveManager->setZoneDuration(zone, durationSec);
}

void WaterManager::setWaterMeterStopThreshold(int ticksPerSecond) {
  EEPROMwl.put(EEPROM_INDEX_WATER_METER_THRESHOLD, ticksPerSecond);
  waterMeter->setThresholdListener(ticksPerSecond, this);
}

void WaterManager::printStatus() {
  Serial.print(F("WaterMeter: "));
  Serial.print(F("used: "));
  Serial.print(waterMeter->getTotalCount());
  Serial.print(F(", stop threshold: "));
  Serial.print(waterMeter->getSamplesInInterval());
  if (stoppedByThreshold > 0) {
    Serial.print(F(", stopped by threshold: "));
    Serial.print(stoppedByThreshold);
  }
  Serial.println();
  valveManager->printStatus();
}

void WaterManager::modeClicked() {
  if (valveManager->isOn()) {
    valveManager->stopAll();
  } else {
    // first show current state and then change it
    // LOW == LED active
    if (digitalRead(MODE_COLOR_GREEN_PIN) == LOW
        || digitalRead(MODE_COLOR_RED_PIN) == LOW
        || digitalRead(MODE_COLOR_BLUE_PIN) == LOW) {
      stoppedByThreshold = 0;
      modeFsm->immediatelyChangeToNextState();
    }
  }
  ((ColorLedState&)modeFsm->getCurrentState()).reactivateLed();
}

void WaterManager::startAutomaticRtc() {
  if (modeFsm->isInState(*modeAutomatic)) {
    valveManager->startAutomaticWithWarn();
  } else if (modeFsm->isInState(*modeOffOnce)) {
    modeFsm->changeState(*modeAutomatic);
  } else {
    Serial.println(F("startAutomaticRtc() ignored due to current mode"));
  }
  ((ColorLedState&)modeFsm->getCurrentState()).reactivateLed();
}

void WaterManager::startAutomatic() {
  valveManager->startAutomatic();
}

void WaterManager::startManual() {
  valveManager->startManual();
}

unsigned int WaterManager::getUsedWater() {
  return waterMeter->getTotalCount();
}

