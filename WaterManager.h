
#ifndef WATER_MANAGER_H
#define WATER_MANAGER_H

#include "Arduino.h"
#include "DurationFsm.h"

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
      pinMode(pin, OUTPUT);
    }
    void on() {
      //      Serial.print("on ");
      //      Serial.println(pin);
      digitalWrite(pin, HIGH);
    }
    void off() {
      //      Serial.print("off ");
      //      Serial.println(pin);
      digitalWrite(pin, LOW);
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
    DurationState *stateManual;

    boolean stopAllRequested;
};

#endif

