
#ifndef WATER_MANAGER_H
#define WATER_MANAGER_H

#include "Arduino.h"
#include "DurationFsm.h"

#define VALVE1_PIN 4
#define VALVE2_PIN 5
#define VALVE3_PIN 6
#define VALVE4_PIN 7

#define MANUAL_ACTIVATION_TIME_SEC 10

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

    Valve valveMain = Valve(VALVE1_PIN);
    Valve valveArea1 = Valve(VALVE2_PIN);
    Valve valveArea2 = Valve(VALVE3_PIN);
    Valve valveArea3 = Valve(VALVE4_PIN);

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

