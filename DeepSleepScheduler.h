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
  - #define SLEEP_TIME_XXX_CORRECTION: When the CPU wakes up from SLEEP_MODE_PWR_DOWN, it needs some cycles to get active. This is also dependent on
  the used CPU type. Using the constants SLEEP_TIME_15MS_CORRECTION to SLEEP_TIME_8S_CORRECTION you can define more exact values for your
  CPU. Please report values back to me if you do some measuring, thanks.
*/

#ifndef DEEP_SLEEP_SCHEDULER_H
#define DEEP_SLEEP_SCHEDULER_H

// -------------------------------------------------------------------------------------------------
// Definition (usually in H file)
// -------------------------------------------------------------------------------------------------
#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/wdt.h>

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
    // supervision timeout of tasks, default is TIMEOUT_8S. Can be deactivated with NO_SUPERVISION.
    TaskTimeout taskTimeout;
    // show on a LED if the CPU is in sleep mode or not. HIGH = active, LOW = sleeping. Default is NOT_USED.
    byte awakeIndicationPin;

    Scheduler();
    void schedule(void (*callback)());
    void scheduleDelayed(void (*callback)(), unsigned long delayMillis);
    void scheduleAt(void (*callback)(), unsigned long uptimeMillis);
    void scheduleAtFrontOfQueue(void (*callback)());
    // cancel all schedules occurences of this callback
    void removeCallbacks(void (*callback)());
    // aquireNoDeepSleepLock() supports up to 255 locks
    void aquireNoDeepSleepLock();
    void releaseNoDeepSleepLock();
    bool doesDeepSleep();
    // call this method from your loop() method
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

#ifndef LIBCALL_DEEP_SLEEP_SCHEDULER
// -------------------------------------------------------------------------------------------------
// Implementation (usuallly in CPP file)
// -------------------------------------------------------------------------------------------------
Scheduler scheduler = Scheduler();

volatile unsigned long Scheduler::millisInDeepSleep;
volatile unsigned long Scheduler::millisBeforeDeepSleep;
volatile unsigned int Scheduler::wdtSleepTimeMillis;

Scheduler::Scheduler() {
  taskTimeout = TIMEOUT_8S;
  awakeIndicationPin = NOT_USED;

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

void Scheduler::aquireNoDeepSleepLock() {
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
             && currentTask->callback != newTask->callback) {
        currentTask = currentTask->next;
      }
      if (currentTask->callback != newTask->callback) {
        // insert after currentTask
        if (currentTask->next != NULL) {
          newTask->next = currentTask->next;
        }
        currentTask->next = newTask;
      } else {
        // a task with the same callback exists before the location where the new one should be insert
        // ignore the new task
        delete newTask;
      }
    }
  }
  interrupts();
}

void Scheduler::execute() {
  Task* current = first;
  while (current != NULL) {
    Serial.print(current->scheduledUptimeMillis);
    Serial.print(F(": "));
    current->callback();
    current = current->next;
  }
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
    sleep_enable();          // enables the sleep bit, a safety pin
    bool sleep;
    if (first != NULL) {
      sleep = evaluateAndPrepareSleep();
    } else {
      // nothing in the queue
      sleep = true;
      if (doesDeepSleep()) {
        Serial.println(F("infinite sleep")); delay(150);
        wdt_disable();
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
      } else {
        set_sleep_mode(SLEEP_MODE_IDLE);
      }
    }
    if (sleep) {
      if (awakeIndicationPin != NOT_USED) {
        digitalWrite(awakeIndicationPin, LOW);
      }
      sleep_mode(); // here the device is actually put to sleep
      // THE PROGRAM CONTINUES FROM HERE AFTER WAKING UP
      if (awakeIndicationPin != NOT_USED) {
        digitalWrite(awakeIndicationPin, HIGH);
      }
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
  if (taskTimeout != NO_SUPERVISION) {
    wdt_disable();
  }
}

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
  // TODO handle case when interrupt during deep sleep excutes a long task. Then the WD would restart the device
}

ISR (WDT_vect) {
  // WDIE & WDIF is cleared in hardware upon entering this ISR
  Scheduler::isrWdt();
}

#endif // #ifndef LIBCALL_DEEP_SLEEP_SCHEDULER
#endif // #ifndef DEEP_SLEEP_SCHEDULER_H

