#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

#ifndef DEEP_SLEEP_SCHEDULER_H
#define DEEP_SLEEP_SCHEDULER_H

#define SCHEDULE_IMMEDIATELLY 0

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

#define NOT_USED 255

class Task {
    friend class Scheduler;
  public:
    Task(void (*callback)(), const unsigned long scheduledUptimeMillis): scheduledUptimeMillis(scheduledUptimeMillis) {
      Task::callback = callback;
      Task::next = NULL;
    }
  private:
    // 0 is used for immediatelly
    const unsigned long scheduledUptimeMillis;
    void (*callback)();
    Task *next;
};

class Scheduler {
    friend class Task;
  public:
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

    TaskTimeout taskTimeout;
    byte awakeIndicationPin;

    static void isrWdt();
  private:
    void insertTask(Task *task);
    inline unsigned long getSchedulerMillis() {
      return millis() + millisInDeepSleep;
    }
    inline bool evaluateAndPrepareSleep();
    Task *first;
    byte noDeepSleepLocksCount;
    static volatile unsigned long millisInDeepSleep;
    static volatile unsigned long millisBeforeDeepSleep;
    static volatile unsigned int wdtSleepTimeMillis;
};

extern Scheduler scheduler;

#endif

