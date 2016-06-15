
#ifndef DURATION_FSM_H
#define DURATION_FSM_H

#include "Arduino.h"
#include "FiniteStateMachine.h"

#define LIBCALL_DEEP_SLEEP_SCHEDULER
#include "DeepSleepScheduler.h"

#define INFINITE_DURATION 0

//define the functionality of the states
class DurationState: public State {
  public:
    DurationState(const unsigned long minDurationMs, String name): State(name), minDurationMs(minDurationMs), nextState(NULL) {
    }
    DurationState(const unsigned long minDurationMs, String name, State * const superState): State(name, superState), minDurationMs(minDurationMs), nextState(NULL) {
    }
    const unsigned long minDurationMs;
    // nextState as NULL marks a state that is not changed when calling changeToNextStateIfElapsed(). minDurationMs is ignored in that case.
    DurationState *nextState;
};

//define the finite state machine functionality
class DurationFsm: FiniteStateMachine, Runnable {
  public:
    DurationFsm(DurationState& current, String name);
    virtual ~DurationFsm() {};

    // Call this method when not using the scheduler. It changes to the next state immediatelly and returns the new state.
    // If the current state is the last state, it does not change state and returns the current state.
    virtual DurationState& immediatelyChangeToNextState();
    virtual DurationFsm& changeState(DurationState& state);

    virtual DurationState& getCurrentState();
    virtual boolean isInState(DurationState &state) const;

    unsigned long timeInCurrentState();

    // method from Runnable
    void run();
};

#endif

