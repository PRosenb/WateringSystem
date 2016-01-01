
#ifndef WATER_MANAGER_H
#define WATER_MANAGER_H

#include "Arduino.h"
#include "DurationFsm.h"

#define UNUSED 255

#define VALVE1_PIN 4
#define VALVE2_PIN 5
#define VALVE3_PIN 6
#define VALVE4_PIN 7

#define DURATION_WARN_SEC 1
#define DURATION_WAIT_BEFORE_SEC 10
#define DURATION_AUTOMATIC1_SEC 10
#define DURATION_AUTOMATIC2_SEC 10
#define DURATION_AUTOMATIC3_SEC 10
#define DURATION_MANUAL_SEC 10

class Valve {
  public:
    Valve(byte pin) {
      Valve::pin = pin;
      if (pin != UNUSED) {
        pinMode(pin, OUTPUT);
      }
    }
    void on() {
      if (pin != UNUSED) {
        digitalWrite(pin, HIGH);
      }
    }
    void off() {
      if (pin != UNUSED) {
        digitalWrite(pin, LOW);
      }
    }
  private:
    byte pin;
};

class ValveState: public DurationState {
  public:
    ValveState(Valve *valve, unsigned int durationMs, String name): DurationState(durationMs, name) {
      ValveState::valve = valve;
    }
    ValveState(Valve *valve, unsigned int durationMs, String name, DurationState * const superState): DurationState(durationMs, name, superState) {
      ValveState::valve = valve;
    }
    virtual void enter() {
      valve->on();

    }
    virtual void exit() {
      valve->off();
    }
  private:
    Valve *valve;
};


class WaterManager {
  public:
    WaterManager();
    ~WaterManager();
    void manualMainOn();
    void startAutomaticWithWarn();
    void startAutomatic();
    void stopAll();
    // return true if finished
    boolean update();
    void allValvesOff();
  private:
    boolean updateWithoutAllValvesOff();

    Valve *valveMain;
    Valve *valveArea1;
    Valve *valveArea2;
    Valve *valveArea3;

    DurationFsm *fsm;
    DurationState *superStateMainIdle;
    DurationState *superStateMainOn;

    DurationState *stateIdle;
    DurationState *stateWarn;
    DurationState *stateWaitBefore;
    DurationState *stateAutomatic1;
    DurationState *stateAutomatic2;
    DurationState *stateAutomatic3;
    DurationState *stateManual;

    boolean stopAllRequested;
};

#endif

