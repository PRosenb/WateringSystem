
#include "DurationFsm.h"

#define LIBCALL_DEEP_SLEEP_SCHEDULER
#include "DeepSleepScheduler.h"

//FINITE STATE MACHINE

DurationFsm::DurationFsm(DurationState& current, const String name): FiniteStateMachine(current, name) {
}

void DurationFsm::run() {
  immediatelyChangeToNextState();
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

boolean DurationFsm::isInState(DurationState & state) const {
  return FiniteStateMachine::isInState(state);
}

unsigned long DurationFsm::timeInCurrentState() {
  return scheduler.getMillis() - stateChangeTime;
}

//END FINITE STATE MACHINE
