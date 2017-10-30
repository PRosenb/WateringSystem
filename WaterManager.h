
#ifndef VALVE_MANAGER_H
#define VALVE_MANAGER_H

#include "Arduino.h"
#include "DurationFsm.h"
#include "WaterMeter.h"
#include "ValveManager.h"
#include "LedState.h"
#include "Constants.h"

#define PIPE_FILLING_TIME_MS 11000
// free flow around 68 per second
#define DEFAULT_WATER_METER_STOP_THRESHOLD 68U
#define SERIAL_SLEEP_TIMEOUT_MS 120000

class WaterManager: public Runnable {
  public:
    WaterManager();
    ~WaterManager();
    /**
       Switch to next mode. Called when the mode button is pressed.
    */
    void modeClicked();
    /**
       Start automatic watering with first one second on as warning.
       If mode is set to modeOffOnce, it is switched back to modeAutomatic.
    */
    void startAutomaticRtc();
    /**
       Start automatic watering without warning second.
       This is done on button press.
    */
    void startAutomatic();
    /**
       Set and store the duration the given zone will be on persistently.
       @param zone number of the zone to be set, 1, 2 or 3
       @param durationSec duration in seconds how long the zone will be watered on every automatic run
    */
    void setZoneDuration(byte zone, unsigned int durationSec);
    /**
       Set the amount of water meter ticks to stop watering if it is reached or exeeded.
    */
    void setWaterMeterStopThreshold(int ticksPerSecond);
    /**
       return the total ticks count of the water meter since system started.
    */
    unsigned long getUsedWater();
    void printStatus();
    /**
       Do not call from external, used internally only.
    */
    void run();
  private:
    void initModeFsm();
    ValveManager *valveManager;
    WaterMeter *waterMeter;
    unsigned int stoppedByThreshold;

    // ModeFsm
    DurationState *modeOff;
    DurationState *modeAutomatic;
    DurationState *modeOffOnce;
    DurationFsm *modeFsm;

    // leak check callback
    const Runnable * const leakCheckListener = new LeakCheckListener(*this);
    class LeakCheckListener: public Runnable {
      public:
        LeakCheckListener(WaterManager &waterManager): waterManager(waterManager) {}
        void run() {
          waterManager.leakCheckListenerCallback();
        }
      private:
        const WaterManager &waterManager;
    };
    void leakCheckListenerCallback();

    // sensor check callback
    const MeasureStateListener * const waterMeterCheckListener = new WaterMeterCheckListener(*this);
    class WaterMeterCheckListener: public MeasureStateListener {
      public:
        WaterMeterCheckListener(WaterManager &waterManager): waterManager(waterManager) {}
        virtual void measuredResult(unsigned int tickCount) {
          waterManager.waterMeterCheckCallback(tickCount);
        }
      private:
        const WaterManager &waterManager;
    };
    void waterMeterCheckCallback(unsigned int tickCount);
};

#endif

