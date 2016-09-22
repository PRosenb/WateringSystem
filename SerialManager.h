
#ifndef SERIAL_MANAGER_H
#define SERIAL_MANAGER_H

#include <timeLib.h>
#define LIBCALL_DEEP_SLEEP_SCHEDULER
#include "DeepSleepScheduler.h"
#include "WaterManager.h"

#define UNDEFINED -1

class SerialManager: public Runnable {
  public:
    SerialManager(byte bluetoothEnablePin);
    virtual ~SerialManager() {};
    void setWaterManager(WaterManager *waterManager);
    void startSerial(unsigned long durationMs);
    int freeRam();
    void run();
  private:
    WaterManager *waterManager;
    const byte bluetoothEnablePin;
    unsigned long durationMs;
    unsigned long serialLastActiveMillis = 0;
    boolean aquiredWakeLock = false;
    time_t startupTime;

    void printTime(time_t time);
    void handleSetDateTime();
    void handleSetAlarmTime();
    void handleGetAlarmTime(byte alarmNumber);
    void handleWrite();
    void handleStatus();
    void handleSerialInput();
    // helper methods
    void readSerial(char inData[], int inDataLength);
    int serialReadInt(int length);
    void printTwoDigits(unsigned int value);
};

#endif

