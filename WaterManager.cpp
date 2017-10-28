
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

  valveManager = new ValveManager(waterMeter, waterMeterCheckListener, leakCheckListener);

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
  Serial.println(F("Leak detected"));
  valveManager->stopAll();
  modeFsm->changeState(*modeOff);
}

void WaterManager::waterMeterCheckCallback(unsigned int tickCount) {
  Serial.print(F("sensorCheckCallback: "));
  Serial.println(tickCount);
#ifdef CHECK_WATER_METER_AVAILABLE
  if (tickCount == 0) {
    Serial.println(F("Water meter not connected"));
    valveManager->stopAll();
    modeFsm->changeState(*modeOff);
  }
#endif
}

void WaterManager::initModeFsm() {
  pinMode(COLOR_LED_GREEN_PIN, OUTPUT);
  pinMode(COLOR_LED_RED_PIN, OUTPUT);
  pinMode(COLOR_LED_BLUE_PIN, OUTPUT);

  modeOff = new ColorLedState(0, 255, 0, INFINITE_DURATION, 10000, F("modeOff"));
  modeOffOnce = new ColorLedState(0, 255, 255, INFINITE_DURATION, 10000, F("modeOffOnce"));
  modeAutomatic = new ColorLedState(255, 0, 0, INFINITE_DURATION, 10000, F("modeAutomatic"));
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
    if (((ColorLedState&)modeFsm->getCurrentState()).isActive()) {
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

unsigned long WaterManager::getUsedWater() {
  return waterMeter->getTotalCount();
}

