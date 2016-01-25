#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#ifndef DEEP_SLEEP_SCHEDULER_H
#define DEEP_SLEEP_SCHEDULER_H

#define SCHEDULE_IMMEDIATELLY 0
#define NOT_USED 255

#define BUFFER_TIME 2
#define SLEEP_TIME_15MS 15 + 3
#define SLEEP_TIME_30MS 30 + 4
#define SLEEP_TIME_60MS 60 + 7
#define SLEEP_TIME_120MS 120 + 13
#define SLEEP_TIME_250MS 250 + 15
#define SLEEP_TIME_500MS 500 + 28
#define SLEEP_TIME_1S 1000 + 54
#define SLEEP_TIME_2S 2000 + 106
#define SLEEP_TIME_4S 4000 + 209
#define SLEEP_TIME_8S 8000 + 415

enum TaskTimeout {
  TIMEOUT_15Ms,
  TIMEOUT_30MS,
  TIMEOUT_60MS,
  TIMEOUT_120MS,
  TIMEOUT_250MS,
  TIMEOUT_500MS,
  TIMEOUT_1S,
  TIMEOUT_2S,
  TIMEOUT_4S,
  TIMEOUT_8S,
  NO_SUPERVISION
};

class Scheduler {
  public:
    TaskTimeout taskTimeout;
    byte awakeIndicationPin;

    Scheduler();
    void schedule(void (*callback)());
    void scheduleDelayed(void (*callback)(), unsigned long delayMillis);
    void scheduleAt(void (*callback)(), unsigned long uptimeMillis);
    void scheduleAtFrontOfQueue(void (*callback)());
    void removeCallbacks(void (*callback)());
    // aquireNoDeepSleepLock() supports up to 255 locks
    void aquireNoDeepSleepLock();
    void releaseNoDeepSleepLock();
    bool doesDeepSleep();
    void execute();
    // returns the millis since startup where the sleep time was added
    inline unsigned long getMillis() {
      return millis() + millisInDeepSleep;
    }

    // do not call, used by WDT ISR
    static void isrWdt();
  private:
    class Task {
      public:
        Task(void (*callback)(), const unsigned long scheduledUptimeMillis)
          : scheduledUptimeMillis(scheduledUptimeMillis), callback(callback), next(NULL) {
        }
        // 0 is used for immediatelly
        const unsigned long scheduledUptimeMillis;
        void (* const callback)();
        Task *next;
    };
    // variables used in the interrupt
    static volatile unsigned long millisInDeepSleep;
    static volatile unsigned long millisBeforeDeepSleep;
    static volatile unsigned int wdtSleepTimeMillis;

    // first element in the run queue
    Task *first;
    // controls if deep sleep is done, 0 does deep sleep
    byte noDeepSleepLocksCount;

    void insertTask(Task *task);
    inline bool evaluateAndPrepareSleep();
};

extern Scheduler scheduler;

#endif

