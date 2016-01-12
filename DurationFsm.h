
#ifndef DURATION_FSM_H
#define DURATION_FSM_H

#include "Arduino.h"
#include "FiniteStateMachine.h"

//define the functionality of the states
class DurationState: public State {
  public:
    DurationState(const unsigned int minDurationMs, String name): State(name), minDurationMs(minDurationMs), nextState(NULL) {
    }
    DurationState(const unsigned int minDurationMs, String name, State * const superState): State(name, superState), minDurationMs(minDurationMs), nextState(NULL) {
    }
    const unsigned int minDurationMs;
    // nextState as NULL marks a state that is not changed when calling changeToNextStateIfElapsed(). minDurationMs is ignored in that case.
    DurationState *nextState;
};

//define the finite state machine functionality
class DurationFsm: FiniteStateMachine {
  public:
    static DurationFsm *getInstance(DurationState& current, const String name);
    static void deleteInstance();
    
    virtual DurationFsm& changeState(DurationState& state);

    virtual DurationState& getCurrentState();
    virtual boolean isInState(DurationState &state) const;

    unsigned long timeInCurrentState();
    
  private:
    DurationFsm(DurationState& current, String name);
    virtual ~DurationFsm() {};
    // static to be called by the scheduler
    static void timeElapsedStatic();
    void timeElapsed();
    static DurationFsm *instance;
};

#endif

