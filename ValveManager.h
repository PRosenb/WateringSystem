
#ifndef WATER_MANAGER_H
#define WATER_MANAGER_H

#include "Arduino.h"
#include "DurationFsm.h"
#include "WaterMeter.h"

#define UNUSED 255

#define VALVE1_PIN 4
#define VALVE2_PIN 5
#define VALVE3_PIN 6
#define VALVE4_PIN 7

#define EEPROM_INDEX_ZONE1 0
#define EEPROM_INDEX_ZONE2 1
#define EEPROM_INDEX_ZONE3 2

#define DURATION_WARN_SEC 2
#define DURATION_WAIT_BEFORE_SEC 60
#define DEFAULT_DURATION_AUTOMATIC1_SEC 60 * 5
#define DEFAULT_DURATION_AUTOMATIC2_SEC 60 * 5
#define DEFAULT_DURATION_AUTOMATIC3_SEC 60 * 1
#define DURATION_MANUAL_SEC 60 * 5

class Valve {
  public:
    Valve(const byte pin): pin(pin) {
      if (pin != UNUSED) {
        pinMode(pin, OUTPUT);
      }
    }
    virtual void on() {
      if (pin != UNUSED) {
        digitalWrite(pin, HIGH);
      }
    }
    virtual void off() {
      if (pin != UNUSED) {
        digitalWrite(pin, LOW);
      }
    }
    virtual bool isOn() {
      if (pin != UNUSED) {
        return digitalRead(pin) == HIGH;
      } else {
        false;
      }
    }
  private:
    const byte pin;
};

// only one valve can be a MeasuredValve
class MeasuredValve: public Valve {
  public:
    MeasuredValve(byte pin, WaterMeter *waterMeter): waterMeter(waterMeter), Valve(pin) {
    }
    virtual ~MeasuredValve() {
      delete waterMeter;
    }
    virtual void on() {
      waterMeter->start();
      Valve::on();
    }
    virtual void off() {
      boolean wasOn = isOn();
      // still switch if off in any case for security reasons
      Valve::off();
      if (wasOn) {
        waterMeter->stop();
        Serial.print(F("measured: "));
        Serial.println(getTotalCount());
      }
    }
    unsigned long getTotalCount() {
      return waterMeter->getTotalCount();
    }
  private:
    WaterMeter * const waterMeter;
};

class ValveSuperState: public SuperState {
  public:
    ValveSuperState(Valve *valve, String name): valve(valve), SuperState(name) {
    }
    virtual void enter() {
      valve->on();
    }
    virtual void exit() {
      valve->off();
    }
  private:
    Valve * const valve;
};

class ValveState: public DurationState {
  public:
    ValveState(Valve *valve, unsigned long durationMs, String name, SuperState * const superState): valve(valve), DurationState(durationMs, name, superState) {
    }
    virtual void enter() {
      valve->on();
    }
    virtual void exit() {
      valve->off();
    }
  private:
    Valve * const valve;
};


class ValveManager {
  public:
    ValveManager(WaterMeter *waterMeter);
    ~ValveManager();
    void manualMainOn();
    void startAutomaticWithWarn();
    void startAutomatic();
    void stopAll();
    bool isOn();
    void setZoneDuration(byte zone, int duration);
    void printStatus();
  private:
    MeasuredValve *valveMain;
    Valve *valveArea1;
    Valve *valveArea2;
    Valve *valveArea3;

    DurationFsm *fsm;
    SuperState *superStateMainIdle;
    ValveSuperState *superStateMainOn;

    DurationState *stateIdle;
    DurationState *stateWarnAutomatic1;
    DurationState *stateWaitBeforeAutomatic1;
    DurationState *stateAutomatic1;
    DurationState *stateBeforeWarnAutomatic2;
    DurationState *stateWarnAutomatic2;
    DurationState *stateWaitBeforeAutomatic2;
    DurationState *stateAutomatic2;
    DurationState *stateBeforeWarnAutomatic3;
    DurationState *stateWarnAutomatic3;
    DurationState *stateWaitBeforeAutomatic3;
    DurationState *stateAutomatic3;
    DurationState *stateManual;
};

#endif

