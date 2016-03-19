
#include "DurationFsm.h"

#define LIBCALL_DEEP_SLEEP_SCHEDULER
#include "DeepSleepScheduler.h"

//FINITE STATE MACHINE

DurationFsm *DurationFsm::instance;

DurationFsm *DurationFsm::createOrGetTheOneInstanceWithScheduledChangeState(DurationState& current, const String name) {
  if (instance == NULL) {
    instance = new DurationFsm(current, name);
  }
  return instance;
}

DurationFsm *DurationFsm::createInstanceWithoutScheduledChangeState(DurationState& current, const String name) {
  return new DurationFsm(current, name);
}

void DurationFsm::deleteTheOneInstanceWithScheduledChangeState() {
  if (instance != NULL) {
    delete instance;
    instance = NULL;
  }
}

DurationFsm::DurationFsm(DurationState& current, const String name): FiniteStateMachine(current, name) {
}

void DurationFsm::timeElapsedStatic() {
  if (instance != NULL) {
    instance->immediatelyChangeToNextState();
  }
}

DurationState& DurationFsm::immediatelyChangeToNextState() {
  DurationState currentState = getCurrentState();
  if (currentState.nextState != NULL) {
    DurationState *nextState = currentState.nextState;
    while (nextState->nextState != NULL && nextState->minDurationMs == 0) {
      nextState = nextState->nextState;
    }
    changeState(*nextState);
    return *nextState;
  } else {
    if (this == instance) {
      // we do not call changeState() so we need to ensure the callback is cancelled.
      scheduler.removeCallbacks(DurationFsm::timeElapsedStatic);
    }
    return currentState;
  }
}

DurationFsm& DurationFsm::changeState(DurationState& state) {
  if (this == instance) {
    scheduler.removeCallbacks(DurationFsm::timeElapsedStatic);
    if (state.minDurationMs > 0 && state.nextState != NULL) {
      scheduler.scheduleDelayed(DurationFsm::timeElapsedStatic, state.minDurationMs);
    }
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
