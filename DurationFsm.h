/*
  Only one instance with automatic scheduling can be create at a time because it uses a static method to receive callbacks from the scheduler.
  minDurationMs=0 deactivates a state. Ensure all active states have a value greater than 0 even if you call immediatelyChangeToNextState() youself.
*/
#ifndef DURATION_FSM_H
#define DURATION_FSM_H

#include "Arduino.h"
#include "FiniteStateMachine.h"

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
class DurationFsm: FiniteStateMachine {
  public:
    // Creates an instance if none exists yet. Otherwise the paramters are ignored and the existing instance is returned.
    static DurationFsm *createOrGetTheOneInstanceWithScheduledChangeState(DurationState& current, const String name);
    static DurationFsm *createInstanceWithoutScheduledChangeState(DurationState& current, const String name);
    static void deleteTheOneInstanceWithScheduledChangeState();

    // Call this method when not using the scheduler. It changes to the next state immediatelly and returns the new state.
    // If the current state is the last state, it does not change state and returns the current state.
    virtual DurationState& immediatelyChangeToNextState();
    virtual DurationFsm& changeState(DurationState& state);

    virtual DurationState& getCurrentState();
    virtual boolean isInState(DurationState &state) const;

    unsigned long timeInCurrentState();

  private:
    DurationFsm(DurationState& current, String name);
    virtual ~DurationFsm() {};
    // static to be called by the scheduler
    static void timeElapsedStatic();
    static DurationFsm *instance;
};

#endif

