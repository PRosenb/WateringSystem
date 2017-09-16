
#include "DurationFsm.h"

//DURATION FSM
#define LIBCALL_DEEP_SLEEP_SCHEDULER
#include <DeepSleepScheduler.h> // https://github.com/PRosenb/DeepSleepScheduler

DurationFsm::DurationFsm(DurationState& current, const String name): FiniteStateMachine(current, name) {
  if (current.minDurationMs > 0 && current.nextState != NULL) {
    scheduler.scheduleDelayed(this, current.minDurationMs);
  }
}

DurationState& DurationFsm::immediatelyChangeToNextState() {
  DurationState currentState = getCurrentState();
  if (currentState.nextState != NULL) {
    DurationState *nextState = currentState.nextState;
    changeState(*nextState);
    return *nextState;
  } else {
    // we do not call changeState() so we need to ensure the callback is cancelled.
    scheduler.removeCallbacks(this);
    return currentState;
  }
}

DurationFsm& DurationFsm::changeState(DurationState& state) {
  scheduler.removeCallbacks(this);
  if (state.minDurationMs > 0 && state.nextState != NULL) {
    scheduler.scheduleDelayed(this, state.minDurationMs);
  }
  DurationState& previousState = getCurrentState();
  FiniteStateMachine::changeState(state);
  if (&previousState != &state) {
    stateChangeTime = scheduler.getMillis();
  }
  return *this;
}

DurationState& DurationFsm::getCurrentState() {
  return (DurationState&) FiniteStateMachine::getCurrentState();
}

boolean DurationFsm::isInState(DurationState& state) const {
  return FiniteStateMachine::isInState(state);
}

boolean DurationFsm::isInState(SuperState& superState) const {
  return FiniteStateMachine::isInState(superState);
}

unsigned long DurationFsm::timeInCurrentState() {
  return scheduler.getMillis() - stateChangeTime;
}

void DurationFsm::run() {
  immediatelyChangeToNextState();
}

//END DURATION FSM
