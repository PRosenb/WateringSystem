
#ifndef FINITESTATEMACHINE_H
#define FINITESTATEMACHINE_H

#include "Arduino.h"

//define the functionality of the states
class State {
  public:
    State(String name): name(name), superState(0) {
    }
    // supports one level of super states. Super states of super states are ignored.
    State(String name, State *const superState): name(name), superState(superState) {
    }
    virtual ~State() {}
    virtual void enter() {}
    virtual void exit() {}
    const String name;
    State * const superState;
};

//define the finite state machine functionality
class FiniteStateMachine {
  public:
    FiniteStateMachine(State& current, String name);

    FiniteStateMachine& changeState(State& state);

    State& getCurrentState();
    boolean isInState(State &state) const;

    unsigned long timeInCurrentState();

  private:
    State* currentState;
    unsigned long stateChangeTime;
    const String name;
};

#endif

