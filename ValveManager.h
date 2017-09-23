
#ifndef WATER_MANAGER_H
#define WATER_MANAGER_H

#include "Arduino.h"
#include "DurationFsm.h"
#include "WaterMeter.h"
#include "Constants.h"

#define UNUSED 255

#define DURATION_WARN_SEC 2U
#define DURATION_WAIT_BEFORE_SEC 60U
#define DURATION_LEAK_CHECK_FILL_MS 3000U
#define DURATION_LEAK_CHECK_WAIT_MS 2000U
#define DEFAULT_DURATION_AUTOMATIC1_SEC 60U * 5U
#define DEFAULT_DURATION_AUTOMATIC2_SEC 60U * 5U
#define DEFAULT_DURATION_AUTOMATIC3_SEC 60U * 1U
#define DURATION_MANUAL_SEC 60U * 5U
#define MAX_ZONE_DURATION 3600U

/**
   Definition of a valve with its PIN. Can be switched on/off and queried on its state.
*/
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

/**
   Extends a normal Valve and adds the functionality to start/stop measuring its water flow when it is switched on/off.
   The main valve is used for this purpose.
   Only one valve can be a MeasuredValve.
*/
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
    const WaterMeter * const waterMeter;
};

/**
   A superState to switch a value on/off when it enters/exits
*/
class ValveSuperState: public SuperState {
  public:
    ValveSuperState(const Valve * const valve, String name): valve(valve), SuperState(name) {
    }
    virtual void enter() {
      valve->on();
    }
    virtual void exit() {
      valve->off();
    }
  private:
    const Valve * const valve;
};

/**
   A duration state to switch a valve on and later off again after a given delay.
*/
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
    const Valve * const valve;
};

/**
   A state which checks if there are any water meter ticks while it is active.
*/
class LeakCheckState: public DurationState, public Runnable {
  public:
    LeakCheckState(unsigned long durationMs, String name, SuperState * const superState, const Runnable * const listener, const WaterMeter *waterMeter):
      listener(listener), waterMeter(waterMeter), DurationState(durationMs, name, superState) {
    }
    virtual void enter() {
      startTotalCount = waterMeter->getTotalCount();
      scheduler.scheduleDelayed(this, 100);
    }
    virtual void exit() {
      scheduler.removeCallbacks(this);
      checkLeak();
    }
    void run() {
      scheduler.scheduleDelayed(this, 100);
      checkLeak();
    }
  private:
    const WaterMeter * const waterMeter;
    const Runnable * const listener;
    unsigned long startTotalCount;
    void checkLeak() {
      if (startTotalCount != waterMeter->getTotalCount()) {
        scheduler.removeCallbacks(this);
        scheduler.schedule(listener);
        Serial.print(F("Leak count: "));
        Serial.println(waterMeter->getTotalCount() - startTotalCount);
      }
    }
};

/**
   A state which checks if there are any water meter ticks while it is active.
*/
class MeasureStateListener {
  public:
    virtual void measuredResult(unsigned int tickCount) = 0;
};
class MeasureState: public DurationState {
  public:
    MeasureState(const Valve *valve, const unsigned long durationMs, const String name, SuperState * const superState, const MeasureStateListener * const listener, const WaterMeter *waterMeter):
      valve(valve), listener(listener), waterMeter(waterMeter), DurationState(durationMs, name, superState) {
    }
    virtual void enter() {
      startTotalCount = waterMeter->getTotalCount();
      if (valve != NULL) {
        valve->on();
      }
    }
    virtual void exit() {
      if (valve != NULL) {
        valve->off();
      }
      unsigned int tickCount = waterMeter->getTotalCount() - startTotalCount;
      listener->measuredResult(tickCount);
    }
  private:
    const Valve * const valve;
    const WaterMeter * const waterMeter;
    const MeasureStateListener * const listener;
    unsigned long startTotalCount;
};


class ValveManager {
  public:
    /**
       @param waterMeter the WaterMeter to use for the measured valve. Cannot be null.
       @param sensorCheckListener callback to report how many ticks the sensor has reported during a period when water should be flowing
       @param leakCheckListener callback executed when a leak is detected (if activated)
    */
    ValveManager(WaterMeter *waterMeter,
                 const MeasureStateListener * const waterMeterCheckListener,
                 const Runnable * const leakCheckListener);
    ~ValveManager();
    /**
       switch manual watering on.
    */
    void startManual();
    /**
       start automated watering with a warn second before the actual watering.
    */
    void startAutomaticWithWarn();
    /**
       start automated watering without a warn second.
    */
    void startAutomatic();
    /**
       Stop watering, switch all valves off.
    */
    void stopAll();
    /**
       returns true if any watering is currently running. Can also be in a waiting state. False if currently idle.
    */
    bool isOn();
    /**
      Set and store the duration the given zone will be on persistently.
      @param zone number of the zone to be set, 1, 2 or 3
      @param durationSec duration in seconds how long the zone will be watered on every automatic run
    */
    void setZoneDuration(byte zone, unsigned int durationSec);
    /**
      print the status of ValveManager to serial.
    */
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
    DurationState *stateLeakCheckFill;
    DurationState *stateLeakCheckWait;
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

