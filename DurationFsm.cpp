
#include "DurationFsm.h"
#include "DeepSleepScheduler.h"

//FINITE STATE MACHINE

DurationFsm *DurationFsm::instance;

DurationFsm *DurationFsm::getInstance(DurationState& current, const String name) {
  if (instance == NULL) {
    instance = new DurationFsm(current, name);
  }
  return instance;
}

void DurationFsm::deleteInstance() {
  if (instance != NULL) {
    delete instance;
    instance = NULL;
  }
}

DurationFsm::DurationFsm(DurationState& current, const String name): FiniteStateMachine(current, name) {
}

void DurationFsm::timeElapsedStatic() {
  instance->timeElapsed();
}

void DurationFsm::timeElapsed() {
  DurationState currentState = getCurrentState();
  if (currentState.nextState != NULL) {
    DurationState *nextState = currentState.nextState;
    while (nextState->nextState != NULL && nextState->minDurationMs == 0) {
      nextState = nextState->nextState;
    }
    changeState(*nextState);
  }
}

DurationFsm& DurationFsm::changeState(DurationState& state) {
  scheduler.removeCallbacks(DurationFsm::timeElapsedStatic);
  if (state.minDurationMs > 0 && state.nextState != NULL) {
    scheduler.scheduleDelayed(DurationFsm::timeElapsedStatic, state.minDurationMs);
  }
  return (DurationFsm&) FiniteStateMachine::changeState(state);
}

DurationState& DurationFsm::getCurrentState() {
  return (DurationState&) FiniteStateMachine::getCurrentState();
}

boolean DurationFsm::isInState(DurationState &state) const {
  return FiniteStateMachine::isInState(state);
}

unsigned long DurationFsm::timeInCurrentState() {
  return FiniteStateMachine::timeInCurrentState();
}

//END FINITE STATE MACHINE
