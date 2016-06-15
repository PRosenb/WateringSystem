
#ifndef FINITESTATEMACHINE_H
#define FINITESTATEMACHINE_H

#include "Arduino.h"

//define the functionality of the states
class SuperState {
  public:
    SuperState(String name): name(name) {
    }
    virtual ~SuperState() {}
    virtual void enter() {}
    virtual void exit() {}
    const String name;
};

class State {
  public:
    State(String name): name(name), superState(NULL) {
    }
    State(String name, SuperState *const superState): name(name), superState(superState) {
    }
    virtual ~State() {}
    virtual void enter() {}
    virtual void exit() {}
    const String name;
    SuperState * const superState;
};

//define the finite state machine functionality
class FiniteStateMachine {
  public:
    FiniteStateMachine(State& current, String name);

    FiniteStateMachine& changeState(State& state);

    State& getCurrentState();
    boolean isInState(State &state) const;

    unsigned long timeInCurrentState();

  protected:
    unsigned long stateChangeTime;

  private:
    State* currentState;
    const String name;
};

#endif

