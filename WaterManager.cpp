
#include "WaterManager.h"
#include <Time.h>         //http://www.arduino.cc/playground/Code/Time

WaterManager::WaterManager() {
  superStateMainIdle = new DurationState(0, "mainIdle");
  superStateMainOn = new ValveState(&valveMain, 0, "mainOn");

  stateIdle = new DurationState(0, "areasIdle", superStateMainIdle);
  stateAutomatic1 = new ValveState(&valveArea1, 10000, "area1", superStateMainOn);
  stateAutomatic2 = new ValveState(&valveArea2, 10000, "area2", superStateMainOn);
  stateManual = new DurationState(10000, "manual", superStateMainOn);

  stateAutomatic1->nextState = stateAutomatic2;
  stateAutomatic2->nextState = stateIdle;
  stateManual->nextState = stateIdle;

  fsm = new DurationFsm(*stateIdle, "FSM");
}

WaterManager::~WaterManager() {
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

void WaterManager::startAutomatic() {
  // ignore if on manual
  if (!fsm->isInState(*stateManual)) {
    fsm->changeState(*stateAutomatic1);
  }
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
  valveMain.off();
  valveArea1.off();
  valveArea2.off();
  valveArea3.off();
}

