
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
#define DEFAULT_WATER_METER_STOP_THRESHOLD 68
#define SERIAL_SLEEP_TIMEOUT_MS 120000

class WaterManager: public Runnable {
  public:
    WaterManager();
    ~WaterManager();
    void modeClicked();
    void startAutomaticRtc();
    void startAutomatic();
    void startManual();
    void setZoneDuration(byte zone, int duration);
    void setWaterMeterStopThreshold(int ticksPerSecond);
    unsigned int getUsedWater();
    void printStatus();
    void run();
  private:
    void initModeFsm();
    ValveManager *valveManager;
    WaterMeter *waterMeter;

    // ModeFsm    
    DurationState *modeOff;
    DurationState *modeAutomatic;
    DurationState *modeOffOnce;
    DurationFsm *modeFsm;
};

#endif

