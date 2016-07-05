
#ifndef VALVE_MANAGER_H
#define VALVE_MANAGER_H

#include "Arduino.h"
#include "DurationFsm.h"
#include "WaterMeter.h"
#include "ValveManager.h"
#include "LedState.h"

#define PIPE_FILLING_TIME_MS 11000
// free flow around 68 per second
#define WATER_METER_STOP_THRESHOLD 68
#define SERIAL_SLEEP_TIMEOUT_MS 30000

#define MODE_COLOR_GREEN_PIN A0
#define MODE_COLOR_RED_PIN A1
#define MODE_COLOR_BLUE_PIN A2

class WaterManager: public Runnable {
  public:
    WaterManager();
    ~WaterManager();
    void modeClicked();
    void startAutomaticRtc();
    void startAutomatic();
    void startManual();
    unsigned int getUsedWater();
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

