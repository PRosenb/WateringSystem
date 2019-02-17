
#ifndef SERIAL_MANAGER_H
#define SERIAL_MANAGER_H

#include <timeLib.h>
#define LIBCALL_DEEP_SLEEP_SCHEDULER
#include <DeepSleepScheduler.h> // https://github.com/PRosenb/DeepSleepScheduler
#include "WaterManager.h"

#define SERIAL_SLEEP_TIMEOUT_MS_DEFAULT 120000
#define UNDEFINED 255

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
    */
    void startSerial();
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
    unsigned long serialLastActiveMillis = 0;
    boolean aquiredWakeLock = false;
    time_t startupTime;

    void printTime(time_t time);
    void handleSetDateTime();
    /**
       sets the alarm1 time.
    */
    void handleSetAlarmTime();
#ifdef RTC_SUPPORTS_READ_ALARM
    /**
       @param alarmNumber identifier of the alarm to print, can be 1 or 2.
    */
    void handleGetAlarmTime(byte alarmNumber);
#endif // RTC_SUPPORTS_READ_ALARM
    void handleWrite();
    void handleStatus();
    void handleSerialInput();
    // helper methods
    void readSerial(char inData[], int inDataLength);
    int serialReadInt(int length);
    void printTwoDigits(unsigned int value);
};

#endif

