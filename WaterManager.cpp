
#include "WaterManager.h"
#include <Time.h>         //http://www.arduino.cc/playground/Code/Time

WaterManager::WaterManager() {
  valveMain = new Valve(VALVE1_PIN);
  valveArea1 = new Valve(VALVE2_PIN);
  valveArea2 = new Valve(VALVE3_PIN);
  valveArea3 = new Valve(VALVE4_PIN);
  stopAllRequested = false;

  superStateMainIdle = new DurationState(0, "mainIdle");
  superStateMainOn = new ValveState(valveMain, 0, "mainOn");

  stateIdle = new DurationState(0, "areasIdle", superStateMainIdle);
  stateWarn = new ValveState(valveArea1, DURATION_WARN_SEC * 1000, "warn", superStateMainOn);
  stateWaitBefore = new DurationState(DURATION_WAIT_BEFORE_SEC * 1000, "areasIdle", superStateMainIdle);
  stateAutomatic1 = new ValveState(valveArea1, DURATION_AUTOMATIC1_SEC * 1000, "area1", superStateMainOn);
  stateAutomatic2 = new ValveState(valveArea2, DURATION_AUTOMATIC2_SEC * 1000, "area2", superStateMainOn);
  stateManual = new DurationState(DURATION_MANUAL_SEC * 1000, "manual", superStateMainOn);

  stateWarn->nextState = stateWaitBefore;
  stateWaitBefore->nextState = stateAutomatic1;
  stateAutomatic1->nextState = stateAutomatic2;
  stateAutomatic2->nextState = stateIdle;
  stateManual->nextState = stateIdle;

  fsm = new DurationFsm(*stateIdle, "FSM");
}

WaterManager::~WaterManager() {
  delete valveMain;
  delete valveArea1;
  delete valveArea2;
  delete valveArea3;

  delete superStateMainIdle;
  delete superStateMainOn;
  delete stateIdle;
  delete stateAutomatic1;
  delete stateAutomatic2;
  delete stateManual;
  delete fsm;
}

void WaterManager::manualMainOn() {
  fsm->changeState(*stateManual);
}

void WaterManager::stopAll() {
  stopAllRequested = true;
  fsm->changeState(*stateIdle);
  allValvesOff();
}

void WaterManager::startAutomaticWithWarn() {
  // ignore if on manual
  if (!fsm->isInState(*stateManual)) {
    fsm->changeState(*stateWarn);
  }
}

void WaterManager::startAutomatic() {
  fsm->changeState(*stateAutomatic1);
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

  if (!fsm->isInState(*stateIdle)) {
    fsm->changeToNextStateIfElapsed();
  }

  return fsm->isInState(*stateIdle);
}

void WaterManager::allValvesOff() {
  valveMain->off();
  valveArea1->off();
  valveArea2->off();
  valveArea3->off();
}

