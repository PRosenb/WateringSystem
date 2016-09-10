
#ifndef SERIAL_MANAGER_H
#define SERIAL_MANAGER_H

#define LIBCALL_DEEP_SLEEP_SCHEDULER
#include "DeepSleepScheduler.h"
#include "WaterManager.h"

#define UNDEFINED -1

class SerialManager: public Runnable {
  public:
    SerialManager(WaterManager *waterManager, byte bluetoothEnablePin);
    virtual ~SerialManager() {};
    void startSerial(unsigned long durationMs);
    int freeRam();
    void run();
  private:
    const WaterManager* waterManager;
    const byte bluetoothEnablePin;
    unsigned long durationMs;
    unsigned long serialLastActiveMillis = 0;
    boolean aquiredWakeLock = false;

    void printTime();
    void handleSetDateTime();
    void handleSetAlarmTime();
    void handleGetAlarmTime(byte alarmNumber);
    void handleWrite();
    void handleSerialInput();
    // helper methods
    void readSerial(char inData[], int inDataLength);
    int serialReadInt(int length);
    void printTwoDigits(unsigned int value);
};

#endif

