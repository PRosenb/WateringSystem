
#include "WaterManager.h"
#include <Time.h>         //http://www.arduino.cc/playground/Code/Time

WaterManager::WaterManager() {
  stateMainIdle = new State("mainIdle");
  stateMainAutomatic = new ValveState(&valveMain, "mainAutomatic");
  stateMainManual = new ValveState(&valveMain, "mainManual");
  mainFsm = new FiniteStateMachine(*stateMainIdle, "Main");

  stateAreasIdle = new State("areasIdle");
  stateAreasAutomatic1 = new ValveState(&valveArea1, "area1");
  stateAreasAutomatic2 = new ValveState(&valveArea2, "area2");
  areasFsm = new FiniteStateMachine(*stateAreasIdle, "Areas");
}

WaterManager::~WaterManager() {
  delete stateMainIdle;
  delete stateMainAutomatic;
  delete stateMainManual;
  delete mainFsm;

  delete stateAreasIdle;
  delete stateAreasAutomatic1;
  delete stateAreasAutomatic2;
  delete areasFsm;
}

void WaterManager::manualMainOn() {
  mainFsm->changeState(*stateMainManual);
  areasFsm->changeState(*stateAreasIdle);
}

void WaterManager::stopAll() {
  stopAllRequested = true;
  mainFsm->changeState(*stateMainIdle);
  areasFsm->changeState(*stateAreasIdle);
  allValvesOff();
}

boolean WaterManager::update() {
  const boolean finished = updateWithoutAllValvesOff();
  if (finished) {
    allValvesOff();
  }
  return finished;
}

boolean WaterManager::updateWithoutAllValvesOff() {
  if (stopAllRequested) {
    stopAllRequested = false;
    return true;
  }

  boolean finished = false;
  if (mainFsm->isInState(*stateMainManual)) {
    // manual
    if (mainFsm->timeInCurrentState() > MANUAL_ACTIVATION_TIME_SEC * 1000) {
      mainFsm->changeState(*stateMainIdle);
      finished = true;
    }
  } else {
    // automatic
    byte sec = second();
    if (sec >= 0 && sec < 10) {
      mainFsm->changeState(*stateMainAutomatic);
      areasFsm->changeState(*stateAreasAutomatic1);
    } else if (sec >= 10 && sec < 20) {
      mainFsm->changeState(*stateMainAutomatic);
      areasFsm->changeState(*stateAreasAutomatic2);
    } else if (sec >= 20 && sec < 30) {
      mainFsm->changeState(*stateMainAutomatic);
      areasFsm->changeState(*stateAreasIdle);
    } else {
      mainFsm->changeState(*stateMainIdle);
      areasFsm->changeState(*stateAreasIdle);
      finished = true;
    }
  }

  return finished;
}

void WaterManager::allValvesOff() {
  valveMain.off();
  valveArea1.off();
  valveArea2.off();
  valveArea3.off();
}

