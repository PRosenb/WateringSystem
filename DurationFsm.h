
#ifndef DURATION_FSM_H
#define DURATION_FSM_H

#include "Arduino.h"
#include "FiniteStateMachine.h"

//define the functionality of the states
class DurationState: public State {
  public:
    DurationState(const unsigned int durationMs, String name): State(name), durationMs(durationMs), nextState(0) {
    }
    DurationState(const unsigned int durationMs, String name, State * const superState): State(name, superState), durationMs(durationMs), nextState(0) {
    }
    const unsigned int durationMs;
    DurationState *nextState;
};

//define the finite state machine functionality
class DurationFsm: FiniteStateMachine {
  public:
    DurationFsm(DurationState& current, String name);
    virtual ~DurationFsm() {};

    virtual DurationFsm& changeState(DurationState& state);
    virtual DurationFsm& changeToNextStateIfElapsed();

    virtual DurationState& getCurrentState();
    virtual boolean isInState(DurationState &state) const;

    unsigned long timeInCurrentState();
};

#endif

