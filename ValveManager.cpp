
#include "ValveManager.h"
#include <Time.h>         // http://www.arduino.cc/playground/Code/Time

ValveManager::ValveManager(WaterMeter *waterMeter) {
  valveMain = new MeasuredValve(VALVE1_PIN, waterMeter);
  valveArea1 = new Valve(VALVE2_PIN);
  valveArea2 = new Valve(VALVE3_PIN);
  valveArea3 = new Valve(VALVE4_PIN);

  superStateMainIdle = new SuperState("mainIdle");
  superStateMainOn = new ValveSuperState(valveMain, "mainOn");

  stateIdle = new DurationState(0, "areasIdle", superStateMainIdle);
  stateWarnAutomatic1 = new ValveState(valveArea1, (unsigned long) DURATION_WARN_SEC * 1000, "warn", superStateMainOn);
  stateWaitBeforeAutomatic1 = new DurationState((unsigned long) DURATION_WAIT_BEFORE_SEC * 1000, "areasIdle", superStateMainIdle);
  stateAutomatic1 = new ValveState(valveArea1, (unsigned long) DURATION_AUTOMATIC1_SEC * 1000, "area1", superStateMainOn);
  // required to switch main valve off in between. Otherwise, the TaskMeter threashold is hit when filling pipe
  stateBeforeWarnAutomatic2 = new DurationState(1000, "beforeWarnArea2", superStateMainIdle);
  stateWarnAutomatic2 = new ValveState(valveArea2, (unsigned long) DURATION_WARN_SEC * 1000, "warnArea2", superStateMainOn);
  stateWaitBeforeAutomatic2 = new DurationState((unsigned long) DURATION_WAIT_BEFORE_SEC * 1000, "areasIdle", superStateMainIdle);
  stateAutomatic2 = new ValveState(valveArea2, (unsigned long) DURATION_AUTOMATIC2_SEC * 1000, "area2", superStateMainOn);
  // required to switch main valve off in between. Otherwise, the TaskMeter threashold is hit when filling pipe
  stateBeforeWarnAutomatic3 = new DurationState(1000, "beforeWarnArea3", superStateMainIdle);
  stateWarnAutomatic3 = new ValveState(valveArea3, (unsigned long) DURATION_WARN_SEC * 1000, "warnArea3", superStateMainOn);
  stateWaitBeforeAutomatic3 = new DurationState((unsigned long) DURATION_WAIT_BEFORE_SEC * 1000, "areasIdle", superStateMainIdle);
  stateAutomatic3 = new ValveState(valveArea3, (unsigned long) DURATION_AUTOMATIC2_SEC * 1000, "area3", superStateMainOn);
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
  fsm->changeState(*stateAutomatic1);
}

bool ValveManager::isOn() {
  return !fsm->isInState(*stateIdle);
}

