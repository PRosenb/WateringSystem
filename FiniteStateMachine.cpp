
#include "FiniteStateMachine.h"

//FINITE STATE MACHINE
FiniteStateMachine::FiniteStateMachine(State& current, const String name): name(name) {
  currentState = &current;
  currentState->enter();
  stateChangeTime = millis();
}

FiniteStateMachine& FiniteStateMachine::changeState(State& state) {
  if (currentState != &state) {
    currentState->exit();

    Serial.print(name);
    Serial.print(F(": changeState: "));
    Serial.print(currentState->name);
    Serial.print(F(" -> "));
    Serial.print(state.name);

    boolean differentSuperStates = currentState->superState != state.superState;
    if (differentSuperStates && currentState->superState != NULL) {
      Serial.print(F(", SUPER: "));
      Serial.print(currentState->superState->name);
      Serial.print(F(" -> "));
      if (state.superState != NULL) {
        Serial.print(currentState->superState->name);
      }
      Serial.println();

      currentState->superState->exit();
    } else {
      Serial.println();
    }

    currentState = &state;
    if (differentSuperStates && state.superState != NULL) {
      currentState->superState->enter();
    }
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
