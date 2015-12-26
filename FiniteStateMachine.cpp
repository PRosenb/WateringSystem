
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
    Serial.print(state.name);

    currentState->exit();
    boolean differentSuperStates = currentState->superState != state.superState;
    if (differentSuperStates && currentState->superState != 0) {
      Serial.print(" : super ");
      Serial.print(currentState->superState->name);
      Serial.print(" -> ");
      currentState->superState->exit();
    }
    currentState = &state;
    if (differentSuperStates && currentState->superState != 0) {
      Serial.print(currentState->superState->name);
      currentState->superState->enter();
    }
    Serial.println();
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
