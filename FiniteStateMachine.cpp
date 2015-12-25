
#include "FiniteStateMachine.h"

//FINITE STATE MACHINE
FiniteStateMachine::FiniteStateMachine(State& current, const String name): name(name) {
  currentState = &current;
  currentState->enter();
  stateChangeTime = millis();
}

FiniteStateMachine& FiniteStateMachine::changeState(State& state) {
  if (currentState != &state) {
    Serial.print(name);
    Serial.print(" changeState ");
    Serial.print(currentState->name);
    Serial.print(" -> ");
    Serial.println(state.name);

    currentState->exit();
    currentState = &state;
    currentState->enter();
    stateChangeTime = millis();
  }
  return *this;
}

//return the current state
State& FiniteStateMachine::getCurrentState() {
  return *currentState;
}

//check if state is equal to the currentState
boolean FiniteStateMachine::isInState(State &state) const {
  if (&state == currentState) {
    return true;
  } else {
    return false;
  }
}

unsigned long FiniteStateMachine::timeInCurrentState() {
  return millis() - stateChangeTime;
}
//END FINITE STATE MACHINE
