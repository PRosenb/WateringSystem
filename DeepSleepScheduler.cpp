#include "DeepSleepScheduler.h"

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

    //    if (first != NULL) {
    //      Serial.print(F("has more in "));
    //      Serial.println(first->scheduledUptimeMillis - getMillis());
    //    }

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
      // allows serial to finish before sleep but leads to wrong sleep duration
      // delay(150);
      if (awakeIndicationPin != NOT_USED) {
        digitalWrite(awakeIndicationPin, LOW);
      }
      sleep_mode();            // here the device is actually put to sleep!!
      if (awakeIndicationPin != NOT_USED) {
        digitalWrite(awakeIndicationPin, HIGH);
      }
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
        wdt_enable(taskTimeout);
      } else {
        wdt_disable();
      }
    } // else the wd is still running

    //    Serial.print(F("millisInDeepSleep "));
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
      // use SLEEP_MODE_IDLE for values less then SLEEP_TIME_1S
    } else if (!doesDeepSleep() || maxWaitTimeMillis < SLEEP_TIME_1S + BUFFER_TIME) {
      sleep = true;
      set_sleep_mode(SLEEP_MODE_IDLE);
      //      Serial.print(F("SLEEP_MODE_IDLE"));
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
      // increase buffer time for this output
      // Serial.print(F("SLEEP_MODE_PWR_DOWN: "));
      // Serial.println(wdtSleepTimeMillis); delay(BUFFER_TIME);
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

