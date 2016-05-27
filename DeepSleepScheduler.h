/*
    Copyright 2016-2016 Peter Rosenberg

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/
/*
  This file is part of the DeepSleepScheduler library for Arduino.
  Definition and code are in the header file in order to allow the user to configure the library by using defines.

  The following options are available:
  - #define LIBCALL_DEEP_SLEEP_SCHEDULER: This h file can only be included once within a project as it also contains the implementation.
    To use it in multiple files, define LIBCALL_DEEP_SLEEP_SCHEDULER before all include statements except one
  All following options are to be set before the include where no LIBCALL_DEEP_SLEEP_SCHEDULER is defined.
  - #define AWAKE_INDICATION_PIN: Show on a LED if the CPU is active or in sleep mode. HIGH = active, LOW = sleeping.
  - #define SLEEP_TIME_XXX_CORRECTION: When the CPU wakes up from SLEEP_MODE_PWR_DOWN, it needs some cycles to get active. This is also dependent on
    the used CPU type. Using the constants SLEEP_TIME_15MS_CORRECTION to SLEEP_TIME_8S_CORRECTION you can define more exact values for your
    CPU. Please report values back to me if you do some measuring, thanks.
  - #define QUEUE_OVERFLOW_PROTECTION: Prevents, that the same callback is scheduled multiple times what may happen with repeated interrupts. When QUEUE_OVERFLOW_PROTECTION
    is set, the new insert callback is ignored, if an other one is found BEFORE the new one should be insert (due to optimisation).
*/

#ifndef DEEP_SLEEP_SCHEDULER_H
#define DEEP_SLEEP_SCHEDULER_H

// -------------------------------------------------------------------------------------------------
// Definition (usually in H file)
// -------------------------------------------------------------------------------------------------
#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

// values changeable by the user
#ifndef SLEEP_TIME_15MS_CORRECTION
#define SLEEP_TIME_15MS_CORRECTION 3
#endif
#ifndef SLEEP_TIME_30MS_CORRECTION
#define SLEEP_TIME_30MS_CORRECTION 4
#endif
#ifndef SLEEP_TIME_60MS_CORRECTION
#define SLEEP_TIME_60MS_CORRECTION 7
#endif
#ifndef SLEEP_TIME_120MS_CORRECTION
#define SLEEP_TIME_120MS_CORRECTION 13
#endif
#ifndef SLEEP_TIME_250MS_CORRECTION
#define SLEEP_TIME_250MS_CORRECTION 15
#endif
#ifndef SLEEP_TIME_500MS_CORRECTION
#define SLEEP_TIME_500MS_CORRECTION 28
#endif
#ifndef SLEEP_TIME_1S_CORRECTION
#define SLEEP_TIME_1S_CORRECTION 54
#endif
#ifndef SLEEP_TIME_2S_CORRECTION
#define SLEEP_TIME_2S_CORRECTION 106
#endif
#ifndef SLEEP_TIME_4S_CORRECTION
#define SLEEP_TIME_4S_CORRECTION 209
#endif
#ifndef SLEEP_TIME_8S_CORRECTION
#define SLEEP_TIME_8S_CORRECTION 415
#endif

// constants
// =========
#define SLEEP_TIME_15MS 15 + SLEEP_TIME_15MS_CORRECTION
#define SLEEP_TIME_30MS 30 + SLEEP_TIME_30MS_CORRECTION
#define SLEEP_TIME_60MS 60 + SLEEP_TIME_60MS_CORRECTION
#define SLEEP_TIME_120MS 120 + SLEEP_TIME_120MS_CORRECTION
#define SLEEP_TIME_250MS 250 + SLEEP_TIME_250MS_CORRECTION
#define SLEEP_TIME_500MS 500 + SLEEP_TIME_500MS_CORRECTION
#define SLEEP_TIME_1S 1000 + SLEEP_TIME_1S_CORRECTION
#define SLEEP_TIME_2S 2000 + SLEEP_TIME_2S_CORRECTION
#define SLEEP_TIME_4S 4000 + SLEEP_TIME_4S_CORRECTION
#define SLEEP_TIME_8S 8000 + SLEEP_TIME_8S_CORRECTION
#define BUFFER_TIME 2

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
    /**
       Configure the supervision of callbacks. Can be deactivated with NO_SUPERVISION.
       Default: TIMEOUT_8S
    */
    TaskTimeout taskTimeout;

    /**
       Schedule the callback method as soon as possible but after other tasks
       that are to be scheduled immediately and are in the queue already.
       @param callback: the method to be called on the main thread
    */
    void schedule(void (*callback)());

    /**
       Schedule the callback after delayMillis milliseconds.
       @param callback: the method to be called on the main thread
       @param delayMillis: the time to wait in milliseconds until the callback shall be made
    */
    void scheduleDelayed(void (*callback)(), unsigned long delayMillis);

    /**
       Schedule the callback uptimeMillis milliseconds after the device was started.
       @param callback: the method to be called on the main thread
       @param uptimeMillis: the time in milliseconds since the device was started
                            to schedule the callback.
    */
    void scheduleAt(void (*callback)(), unsigned long uptimeMillis);

    /**
       Schedule the callback method as next task even if other tasks are in the queue already.
       @param callback: the method to be called on the main thread
    */
    void scheduleAtFrontOfQueue(void (*callback)());

    /**
       Cancel all schedules that were scheduled for this callback.
       @param callback: method of which all schedules shall be removed
    */
    void removeCallbacks(void (*callback)());

    /**
       Acquire a lock to prevent the CPU from entering deep sleep.
       acquireNoDeepSleepLock() supports up to 255 locks.
       You need to call releaseNoDeepSleepLock() the same amount of times
       as removeCallbacks() to allow the CPU to enter deep sleep again.
    */
    void acquireNoDeepSleepLock();

    /**
       Release the lock acquired by acquireNoDeepSleepLock(). Please make sure you
       call releaseNoDeepSleepLock() the same amount of times as acquireNoDeepSleepLock(),
       otherwise the CPU is not allowed to enter deep sleep.
    */
    void releaseNoDeepSleepLock();

    /**
       return: true if the CPU is currently allowed to enter deep sleep, false otherwise.
    */
    bool doesDeepSleep();

    /**
       return: The milliseconds since startup of the device where the sleep time was added
    */
    inline unsigned long getMillis() {
      return millis() + millisInDeepSleep;
    }

    /**
       This method needs to be called from your loop() method and does not return.
    */
    void execute();

    /**
       Constructor of the scheduler. Do not all this method as there is only one instance of Scheduler supported.
    */
    Scheduler();

    /**
        Do not call this method, it is used by the watchdog interrupt.
    */
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

    /**
       first element in the run queue
    */
    Task *first;
    /**
       controls if deep sleep is done, 0 does deep sleep
    */
    byte noDeepSleepLocksCount;

    void insertTask(Task *task);
    inline bool evaluateAndPrepareSleep();
};

/**
   the one and only instance of Scheduler
*/
extern Scheduler scheduler;

#ifndef LIBCALL_DEEP_SLEEP_SCHEDULER
// -------------------------------------------------------------------------------------------------
// Implementation (usuallly in CPP file)
// -------------------------------------------------------------------------------------------------
Scheduler scheduler = Scheduler();

volatile unsigned long Scheduler::millisInDeepSleep;
volatile unsigned long Scheduler::millisBeforeDeepSleep;
volatile unsigned int Scheduler::wdtSleepTimeMillis;

Scheduler::Scheduler() {
#ifdef AWAKE_INDICATION_PIN
  pinMode(AWAKE_INDICATION_PIN, OUTPUT);
#endif
  taskTimeout = TIMEOUT_8S;

  millisInDeepSleep = 0;
  millisBeforeDeepSleep = 0;
  wdtSleepTimeMillis = 0;

  first = NULL;
  noDeepSleepLocksCount = 0;
}

void Scheduler::schedule(void (*callback)()) {
  Task *newTask = new Task(callback, SCHEDULE_IMMEDIATELLY);
  insertTask(newTask);
}

void Scheduler::scheduleDelayed(void (*callback)(), unsigned long delayMillis) {
  Task *newTask = new Task(callback, getMillis() + delayMillis);
  insertTask(newTask);
}

void Scheduler::scheduleAt(void (*callback)(), unsigned long uptimeMillis) {
  Task *newTask = new Task(callback, uptimeMillis);
  insertTask(newTask);
}

void Scheduler::scheduleAtFrontOfQueue(void (*callback)()) {
  Task *newTask = new Task(callback, SCHEDULE_IMMEDIATELLY);
  noInterrupts();
  newTask->next = first;
  first = newTask;
  interrupts();
}

void Scheduler::acquireNoDeepSleepLock() {
  noDeepSleepLocksCount++;
}
void Scheduler::releaseNoDeepSleepLock() {
  if (noDeepSleepLocksCount != 0) {
    noDeepSleepLocksCount--;
  }
}
bool Scheduler::doesDeepSleep() {
  return noDeepSleepLocksCount == 0;
}

void Scheduler::removeCallbacks(void (*callback)()) {
  noInterrupts();
  if (first != NULL) {
    Task *previousTask = NULL;
    Task *currentTask = first;
    while (currentTask != NULL) {
      if (currentTask->callback == callback) {
        Task *taskToDelete = currentTask;
        if (previousTask == NULL) {
          // remove the first task
          first = taskToDelete->next;
        } else {
          previousTask->next = taskToDelete->next;
        }
        currentTask = taskToDelete->next;
        delete taskToDelete;
      } else {
        previousTask = currentTask;
        currentTask = currentTask->next;
      }
    }
  }
  interrupts();
}

// Inserts a new task in the ordered lists of tasks.
// If there is a task in the list with the same callback
// before the position where the new task is to be insert
// the new task is ignored to prevent queue overflow.
void Scheduler::insertTask(Task *newTask) {
  noInterrupts();
  if (first == NULL) {
    first = newTask;
  } else {
    if (first->scheduledUptimeMillis > newTask->scheduledUptimeMillis) {
      // insert before first
      newTask->next = first;
      first = newTask;
    } else {
      Task *currentTask = first;
      while (currentTask->next != NULL
             && currentTask->next->scheduledUptimeMillis <= newTask->scheduledUptimeMillis
#ifdef QUEUE_OVERFLOW_PROTECTION
             && currentTask->callback != newTask->callback
#endif
            ) {
        currentTask = currentTask->next;
      }
#ifdef QUEUE_OVERFLOW_PROTECTION
      if (currentTask->callback != newTask->callback) {
#endif
        // insert after currentTask
        if (currentTask->next != NULL) {
          newTask->next = currentTask->next;
        }
        currentTask->next = newTask;
#ifdef QUEUE_OVERFLOW_PROTECTION
      } else {
        // a task with the same callback exists before the location where the new one should be insert
        // ignore the new task
        delete newTask;
      }
#endif
    }
  }
  interrupts();
}

void Scheduler::execute() {
  if (taskTimeout != NO_SUPERVISION) {
    wdt_enable(taskTimeout);
  }
  while (true) {
    while (true) {
      Task *current = NULL;
      noInterrupts();
      if (first != NULL && first->scheduledUptimeMillis <= getMillis()) {
        current = first;
        first = current->next;
      }
      interrupts();

      if (current != NULL) {
        wdt_reset();
        current->callback();
        delete current;
      } else {
        break;
      }
    }
    wdt_reset();

    // Enable sleep bit with sleep_enable() before the sleep time evaluation because it can happen
    // that the WDT interrupt occurs during sleep time evaluation but before the CPU
    // sleeps. In that case, the WDT interrupt clears the sleep bit and the CPU will not sleep
    // but continue execution immediatelly.
    sleep_enable(); // enables the sleep bit, a safety pin
    bool sleep;
    if (first != NULL) {
      sleep = evaluateAndPrepareSleep();
    } else {
      // nothing in the queue
      sleep = true;
      if (doesDeepSleep()) {
        wdt_disable();
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
      } else {
        set_sleep_mode(SLEEP_MODE_IDLE);
      }
    }
    if (sleep) {
#ifdef AWAKE_INDICATION_PIN
      digitalWrite(AWAKE_INDICATION_PIN, LOW);
#endif
      sleep_mode(); // here the device is actually put to sleep
      // THE PROGRAM CONTINUES FROM HERE AFTER WAKING UP
#ifdef AWAKE_INDICATION_PIN
      digitalWrite(AWAKE_INDICATION_PIN, HIGH);
#endif
    }
    sleep_disable();

    noInterrupts();
    unsigned long wdtSleepTimeMillisLocal = wdtSleepTimeMillis;
    interrupts();
    if (wdtSleepTimeMillisLocal == 0) {
      // woken up due to WDT interrupt
      if (taskTimeout != NO_SUPERVISION) {
        // change back to taskTimeout
        wdt_reset();
        wdt_enable(taskTimeout);
      } else {
        // tasks are not suppervised, deactivate WDT
        wdt_disable();
      }
    } // else the wd is still running
  }
  // never executed so no need to deactivate the WDT
}

// returns true if sleep SLEEP_MODE_IDLE or SLEEP_MODE_PWR_DOWN is needed
bool Scheduler::evaluateAndPrepareSleep() {
  bool sleep = false;

  noInterrupts();
  unsigned long wdtSleepTimeMillisLocal = wdtSleepTimeMillis;
  interrupts();

  if (wdtSleepTimeMillisLocal == 0) {
    // not woken up during WDT sleep

    unsigned long maxWaitTimeMillis = 0;
    unsigned long currentSchedulerMillis = getMillis();
    noInterrupts();
    unsigned long firstScheduledUptimeMillis = first->scheduledUptimeMillis;
    interrupts();
    if (firstScheduledUptimeMillis > currentSchedulerMillis) {
      maxWaitTimeMillis = firstScheduledUptimeMillis - currentSchedulerMillis;
    }
    if (maxWaitTimeMillis == 0) {
      sleep = false;
    } else if (!doesDeepSleep() || maxWaitTimeMillis < SLEEP_TIME_1S + BUFFER_TIME) {
      // use SLEEP_MODE_IDLE for values less then SLEEP_TIME_1S
      sleep = true;
      set_sleep_mode(SLEEP_MODE_IDLE);
    } else {
      sleep = true;
      set_sleep_mode(SLEEP_MODE_PWR_DOWN);

      if (maxWaitTimeMillis >= SLEEP_TIME_8S + BUFFER_TIME) {
        wdtSleepTimeMillis = SLEEP_TIME_8S;
        wdt_enable(WDTO_8S);
      } else if (maxWaitTimeMillis >= SLEEP_TIME_4S + BUFFER_TIME) {
        wdtSleepTimeMillis = SLEEP_TIME_4S;
        wdt_enable(WDTO_4S);
      } else if (maxWaitTimeMillis >= SLEEP_TIME_2S + BUFFER_TIME) {
        wdtSleepTimeMillis = SLEEP_TIME_2S;
        wdt_enable(WDTO_2S);
      } else if (maxWaitTimeMillis >= SLEEP_TIME_1S + BUFFER_TIME) {
        wdtSleepTimeMillis = SLEEP_TIME_1S;
        wdt_enable(WDTO_1S);
      } else if (maxWaitTimeMillis >= SLEEP_TIME_500MS + BUFFER_TIME) {
        wdtSleepTimeMillis = SLEEP_TIME_500MS;
        wdt_enable(WDTO_500MS);
      } else if (maxWaitTimeMillis >= SLEEP_TIME_250MS + BUFFER_TIME) {
        wdtSleepTimeMillis = SLEEP_TIME_250MS;
        wdt_enable(WDTO_250MS);
      } else if (maxWaitTimeMillis >= SLEEP_TIME_120MS + BUFFER_TIME) {
        wdtSleepTimeMillis = SLEEP_TIME_120MS;
        wdt_enable(WDTO_120MS);
      } else if (maxWaitTimeMillis >= SLEEP_TIME_60MS + BUFFER_TIME) {
        wdtSleepTimeMillis = SLEEP_TIME_60MS;
        wdt_enable(WDTO_60MS);
      } else if (maxWaitTimeMillis >= SLEEP_TIME_30MS + BUFFER_TIME) {
        wdtSleepTimeMillis = SLEEP_TIME_30MS;
        wdt_enable(WDTO_30MS);
      } else { // maxWaitTimeMs >= 17
        wdtSleepTimeMillis = SLEEP_TIME_15MS;
        wdt_enable(WDTO_15MS);
      }

      // enable interrupt
      // first timeout will be the interrupt, second system reset
      WDTCSR |= (1 << WDCE) | (1 << WDIE);
      millisBeforeDeepSleep = millis();
    }
  } else {
    // wdt already running, so we woke up due to an other interrupt
    // continue sleepting without enabling wdt again
    sleep = true;
    WDTCSR |= (1 << WDCE) | (1 << WDIE);
  }
  return sleep;
}

void Scheduler::isrWdt() {
  sleep_disable();
  millisInDeepSleep += wdtSleepTimeMillis;
  wdtSleepTimeMillis = 0;
  millisInDeepSleep -= millis() - millisBeforeDeepSleep;
}

ISR (WDT_vect) {
  // WDIE & WDIF is cleared in hardware upon entering this ISR
  Scheduler::isrWdt();
}

#endif // #ifndef LIBCALL_DEEP_SLEEP_SCHEDULER
#endif // #ifndef DEEP_SLEEP_SCHEDULER_H

