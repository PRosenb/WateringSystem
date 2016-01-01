
#include "DurationFsm.h"

//FINITE STATE MACHINE
DurationFsm::DurationFsm(DurationState& current, const String name): FiniteStateMachine(current, name) {
}

DurationFsm& DurationFsm::changeState(DurationState& state) {
  return (DurationFsm&) FiniteStateMachine::changeState(state);
}

DurationFsm& DurationFsm::changeToNextStateIfElapsed() {
  DurationState currentState = getCurrentState();
  if (currentState.nextState != NULL && FiniteStateMachine::timeInCurrentState() >= currentState.durationMs) {
    DurationState *nextState = currentState.nextState;
    while (nextState->nextState != NULL && nextState->durationMs == 0) {
      nextState = nextState->nextState;
    }
    changeState(*nextState);
  }
  return *this;
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
