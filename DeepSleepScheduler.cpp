#include "DeepSleepScheduler.h"

Scheduler scheduler = Scheduler();

volatile unsigned long Scheduler::millisInDeepSleep;
volatile unsigned long Scheduler::millisBeforeDeepSleep;
volatile unsigned long Scheduler::wdtSleepTimeMillis;

Scheduler::Scheduler() {
  first = NULL;
  millisInDeepSleep = 0;
  deepSleep = true;
  taskTimeout = TIMEOUT_8S;
}

void Scheduler::schedule(void (*callback)()) {
  Task *newTask = new Task(callback, SCHEDULE_IMMEDIATELLY);
  insertTask(newTask);
}

void Scheduler::scheduleFromInterrupt(void (*callback)()) {
//  taskFromInterrupt = new Task(callback, SCHEDULE_IMMEDIATELLY);
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

void Scheduler::insertTask(Task *newTask) {
  noInterrupts();
  if (first == NULL) {
    first = newTask;
  } else {
    Task *currentTask = first;
    while (currentTask->next != NULL && currentTask->next->scheduledUptimeMillis <= newTask->scheduledUptimeMillis) {
      currentTask = currentTask->next;
    }
    if (currentTask->next != NULL) {
      newTask->next = currentTask->next;
    }
    currentTask->next = newTask;
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
      if (deepSleep) {
        // sleep until
        wdt_disable();
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
      } else {
        set_sleep_mode(SLEEP_MODE_IDLE);
      }
    }
    if (sleep) {
      sleep_mode();            // here the device is actually put to sleep!!
    }
    // THE PROGRAM CONTINUES FROM HERE AFTER WAKING UP
    sleep_disable();

    if (taskTimeout != NO_SUPERVISION) {
      // change back to taskTimeout
      wdt_enable(taskTimeout);
    } else {
      wdt_disable();
    }

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
    } else if (!deepSleep || maxWaitTimeMillis < 17) {
      sleep = true;
      set_sleep_mode(SLEEP_MODE_IDLE);
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
      WDTCSR |= (1 << WDIE);
      millisBeforeDeepSleep = millis();
    }
  } else {
    // wdt already running, so we woke up due to an other interrupt
    // continue sleepting without enabling wdt again
    sleep = true;
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

