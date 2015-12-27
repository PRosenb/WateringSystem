
#include "DurationFsm.h"

//FINITE STATE MACHINE
DurationFsm::DurationFsm(DurationState& current, const String name): FiniteStateMachine(current, name) {
}

DurationFsm& DurationFsm::changeState(DurationState& state) {
  return (DurationFsm&) FiniteStateMachine::changeState(state);
}

DurationFsm& DurationFsm::changeToNextStateIfElapsed() {
  if (FiniteStateMachine::timeInCurrentState() >= getCurrentState().durationMs) {
    changeState(*getCurrentState().nextState);
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
