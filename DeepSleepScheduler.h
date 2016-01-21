#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#ifndef DEEP_SLEEP_SCHEDULER_H
#define DEEP_SLEEP_SCHEDULER_H

#define SCHEDULE_IMMEDIATELLY 0
#define NOT_USED 255

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
    void scheduleDelayed(void (*callback)(), int delayMillis);
    void scheduleAt(void (*callback)(), unsigned long uptimeMillis);
    void scheduleAtFrontOfQueue(void (*callback)());
    void removeCallbacks(void (*callback)());
    // aquireNoDeepSleepLock() supports up to 255 locks
    void aquireNoDeepSleepLock();
    void releaseNoDeepSleepLock();
    bool doesDeepSleep();
    void execute();

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
    inline unsigned long getSchedulerMillis() {
      return millis() + millisInDeepSleep;
    }
    inline bool evaluateAndPrepareSleep();
};

extern Scheduler scheduler;

#endif

