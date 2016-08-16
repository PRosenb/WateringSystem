
#ifndef SERIAL_MANAGER_H
#define SERIAL_MANAGER_H

#define LIBCALL_DEEP_SLEEP_SCHEDULER
#include "DeepSleepScheduler.h"

#define UNDEFINED -1

class SerialManager: public Runnable {
  public:
    SerialManager(byte bluetoothEnablePin);
    virtual ~SerialManager() {};
    void startSerial(unsigned long durationMs);
    int freeRam();
    void run();
  private:
    const byte bluetoothEnablePin;
    unsigned long durationMs;
    unsigned long serialLastActiveMillis = 0;
    boolean aquiredWakeLock = false;

    void printTime();
    void handleSetDateTime();
    void handleSetAlarmTime();
    void handleGetAlarmTime(byte alarmNumber);
    void handleSerialInput();
    // helper methods
    void readSerial(char inData[], int inDataLength);
    int serialReadInt(int length);
    void printTwoDigits(unsigned int value);
};

#endif

