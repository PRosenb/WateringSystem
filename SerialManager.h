
#ifndef SERIAL_MANAGER_H
#define SERIAL_MANAGER_H

#include <timeLib.h>
#define LIBCALL_DEEP_SLEEP_SCHEDULER
#include <DeepSleepScheduler.h> // https://github.com/PRosenb/DeepSleepScheduler
#include "WaterManager.h"

#define UNDEFINED -1

class SerialManager: public Runnable {
  public:
    /**
       @param bluetoothEnablePin the PIN where the bluetooth module can be enabled/disabled on or UNDEFINED if not used
    */
    SerialManager(byte bluetoothEnablePin);
    virtual ~SerialManager() {};
    /**
       set waterManager to forward commands to it from console
       @param waterManager the waterManager, must be set, cannot be null
    */
    void setWaterManager(WaterManager *waterManager);
    /**
       start serial communication.
       @param durationMs duration in ms until serial communication is disabled when not used.
    */
    void startSerial(unsigned long durationMs);
    /**
       returns the amount of free ram in bytes.
    */
    int freeRam();
    /**
       do not call from external, used internaly to interact with the serial interface.
    */
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
    /**
       sets the alarm1 time.
    */
    void handleSetAlarmTime();
    /**
       @param alarmNumber identifier of the alarm to print, can be 1 or 2.
    */
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

