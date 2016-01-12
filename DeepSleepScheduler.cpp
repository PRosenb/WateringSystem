#include "DeepSleepScheduler.h"

Scheduler scheduler = Scheduler();

volatile unsigned long Scheduler::millisInDeepSleep;
volatile unsigned long Scheduler::millisBeforeDeepSleep;
volatile unsigned long Scheduler::wdtSleepTimeMillis;

Scheduler::Scheduler() {
  first = NULL;
  millisInDeepSleep = 0;
  noDeepSleepLocksCount = 0;
  taskTimeout = TIMEOUT_8S;
  awakeIndicationPin = NOT_USED;
}

void Scheduler::schedule(void (*callback)()) {
  Task *newTask = new Task(callback, SCHEDULE_IMMEDIATELLY);
  insertTask(newTask);
}

void Scheduler::scheduleDelayed(void (*callback)(), int delayMillis) {
  Task *newTask = new Task(callback, getSchedulerMillis() + delayMillis);
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

// inserts a new task in the ordered lists of tasks
// if there is a task in the list with the same callback
// before the position where the new task is to be insert
// the new task is ignored to prevent list overflow
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
  if (taskTimeout != NO_SUPERVISION) {
    wdt_enable(taskTimeout);
  }
  while (true) {
    while (true) {
      Task *current = NULL;
      noInterrupts();
      if (first != NULL && first->scheduledUptimeMillis <= getSchedulerMillis()) {
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

    //    if (first != NULL) {
    //      Serial.print("has more in ");
    //      Serial.println(first->scheduledUptimeMillis - getSchedulerMillis()); delay(100);
    //    }

    sleep_enable();          // enables the sleep bit, a safety pin
    bool sleep;
    if (first != NULL) {
      sleep = evaluateAndPrepareSleep();
    } else {
      // nothing in the queue
      sleep = true;
      if (doesDeepSleep()) {
        // sleep until
        wdt_disable();
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        Serial.println("infinite sleep");
      } else {
        set_sleep_mode(SLEEP_MODE_IDLE);
      }
    }
    if (sleep) {
      //      Serial.println("before sleep");
      delay(150);
      if (awakeIndicationPin != NOT_USED) {
        digitalWrite(awakeIndicationPin, LOW);
      }
      sleep_mode();            // here the device is actually put to sleep!!
      if (awakeIndicationPin != NOT_USED) {
        digitalWrite(awakeIndicationPin, HIGH);
      }
      //            Serial.println("after sleep");
    }
    // THE PROGRAM CONTINUES FROM HERE AFTER WAKING UP
    sleep_disable();

    noInterrupts();
    unsigned long wdtSleepTimeMillisLocal = wdtSleepTimeMillis;
    interrupts();
    if (wdtSleepTimeMillisLocal == 0) {
      if (taskTimeout != NO_SUPERVISION) {
        // change back to taskTimeout
        wdt_reset();
        wdt_disable();
        wdt_enable(taskTimeout);
        //        Serial.print("to taskTimeout: ");
        //        Serial.println(taskTimeout);
      } else {
        wdt_disable();
      }
    } // else the wd is still running

    //    Serial.print("millisInDeepSleep ");
    //    Serial.println(millisInDeepSleep);
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
    // wdt sleep not enabled

    unsigned long maxWaitTimeMillis = 0;
    unsigned long currentSchedulerMillis = getSchedulerMillis();
    noInterrupts();
    unsigned long firstScheduledUptimeMillis = first->scheduledUptimeMillis;
    interrupts();
    if (firstScheduledUptimeMillis > currentSchedulerMillis) {
      maxWaitTimeMillis = firstScheduledUptimeMillis - currentSchedulerMillis;
    }
    if (maxWaitTimeMillis == 0) {
      sleep = false;
      //            Serial.println("no sleep");
    } else if (!doesDeepSleep() || maxWaitTimeMillis < 998) {
      sleep = true;
      set_sleep_mode(SLEEP_MODE_IDLE);
      //            Serial.println("SLEEP_MODE_IDLE");
    } else {
      sleep = true;
      set_sleep_mode(SLEEP_MODE_PWR_DOWN);

      if (maxWaitTimeMillis >= 8002) {
        wdtSleepTimeMillis = 8000;
        wdt_enable(WDTO_8S);
      } else if (maxWaitTimeMillis >= 4002) {
        wdtSleepTimeMillis = 4000;
        wdt_enable(WDTO_4S);
      } else if (maxWaitTimeMillis >= 2002) {
        wdtSleepTimeMillis = 2000;
        wdt_enable(WDTO_2S);
      } else if (maxWaitTimeMillis >= 998) {
        wdtSleepTimeMillis = 1000;
        wdt_enable(WDTO_1S);
      } else if (maxWaitTimeMillis >= 502) {
        wdtSleepTimeMillis = 500;
        wdt_enable(WDTO_500MS);
      } else if (maxWaitTimeMillis >= 252) {
        wdtSleepTimeMillis = 250;
        wdt_enable(WDTO_250MS);
      } else if (maxWaitTimeMillis >= 122) {
        wdtSleepTimeMillis = 120;
        wdt_enable(WDTO_120MS);
      } else if (maxWaitTimeMillis >= 62) {
        wdtSleepTimeMillis = 60;
        wdt_enable(WDTO_60MS);
      } else if (maxWaitTimeMillis >= 32) {
        wdtSleepTimeMillis = 30;
        wdt_enable(WDTO_30MS);
      } else { // maxWaitTimeMs >= 17
        wdtSleepTimeMillis = 15;
        wdt_enable(WDTO_15MS);
      }

      // enable interrupt
      // first timeout will be the interrupt, second system reset
      WDTCSR |= (1 << WDCE) | (1 << WDIE);
      millisBeforeDeepSleep = millis();
      //      Serial.print("SLEEP_MODE_PWR_DOWN: ");
      //      Serial.println(wdtSleepTimeMillis);
    }
  } else {
    // wdt already running, so we woke up due to an other interrupt
    // continue sleepting without enabling wdt again
    sleep = true;
    WDTCSR |= (1 << WDCE) | (1 << WDIE);
    Serial.println("SLEEP_MODE_PWR_DOWN resleep");
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

