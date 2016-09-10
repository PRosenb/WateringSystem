
#include "WaterManager.h"
#include "LedState.h"

WaterManager::WaterManager() {
  waterMeter = new WaterMeter(1000);
  waterMeter->setThresholdSupervisionDelay(PIPE_FILLING_TIME_MS);
  waterMeter->setThresholdListener(WATER_METER_STOP_THRESHOLD, this);
  valveManager = new ValveManager(waterMeter);

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
  Serial.print(F("ThresholdListener: "));
  Serial.println(waterMeter->getLastPulseCountOverThreshold());
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

  modeOff = new ColorLedState(255, MODE_COLOR_RED_PIN, 255, 10000, "modeOff");
  modeOffOnce = new ColorLedState(255, MODE_COLOR_RED_PIN, MODE_COLOR_BLUE_PIN, 10000, "modeOffOnce");
  modeAutomatic = new ColorLedState(MODE_COLOR_GREEN_PIN, 255, 255, 10000, "modeAutomatic");
  // for simple LED test:
  //  modeOff = new ColorLedState(255, MODE_COLOR_RED_PIN, MODE_COLOR_BLUE_PIN, 10000, "modeOff");
  //  modeOffOnce = new ColorLedState(MODE_COLOR_GREEN_PIN, 255, MODE_COLOR_BLUE_PIN, 10000, "modeOffOnce");
  //  modeAutomatic = new ColorLedState(MODE_COLOR_GREEN_PIN, MODE_COLOR_RED_PIN, 255, 10000, "modeAutomatic");
  modeOff->nextState = modeAutomatic;
  modeAutomatic->nextState = modeOffOnce;
  modeOffOnce->nextState = modeOff;

  modeFsm = new DurationFsm(*modeOff, "ModeFSM");
}

void WaterManager::setZoneDuration(byte zone, int duration) {
  valveManager->setZoneDuration(zone, duration);
}

void WaterManager::printStatus() {
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
  }
  ((ColorLedState&)modeFsm->getCurrentState()).reactivateLed();
}

void WaterManager::startAutomatic() {
  valveManager->startAutomatic();
}

void WaterManager::startManual() {
  valveManager->manualMainOn();
}

unsigned int WaterManager::getUsedWater() {
  return waterMeter->getTotalCount();
}

