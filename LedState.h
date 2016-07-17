
#ifndef LED_STATE_H
#define LED_STATE_H

#include "DurationFsm.h"

class ColorLedState: public DurationState, public Runnable {
  public:
    // the pins to set to LOW on enter, 255 for unused colors
    ColorLedState(byte greenPin, byte redPin, byte bluePin, unsigned long ledOnDurationMs, String name)
      : DurationState(INFINITE_DURATION, name), greenPin(greenPin), redPin(redPin), bluePin(bluePin), ledOnDurationMs(ledOnDurationMs) {
    }
    ColorLedState(byte greenPin, byte redPin, byte bluePin, unsigned long ledOnDurationMs, String name, SuperState * const superState)
      : DurationState(INFINITE_DURATION, name, superState), greenPin(greenPin), redPin(redPin), bluePin(bluePin), ledOnDurationMs(ledOnDurationMs) {
    }
    virtual void enter() {
      reactivateLed();
    }
    virtual void exit() {
      deactivateLed();
      scheduler.removeCallbacks(this);
    }
    void reactivateLed() {
      if (greenPin != 255) {
        digitalWrite(greenPin, LOW);
      }
      if (redPin != 255) {
        digitalWrite(redPin, LOW);
      }
      if (bluePin != 255) {
        digitalWrite(bluePin, LOW);
      }
      if (ledOnDurationMs != INFINITE_DURATION) {
        scheduler.scheduleDelayed(this, ledOnDurationMs);
      }
    }
    void deactivateLed() {
      if (greenPin != 255) {
        digitalWrite(greenPin, HIGH);
      }
      if (redPin != 255) {
        digitalWrite(redPin, HIGH);
      }
      if (bluePin != 255) {
        digitalWrite(bluePin, HIGH);
      }
    }
    void run() {
      deactivateLed();
    }
  private:
    const byte greenPin, redPin, bluePin;
    const unsigned long ledOnDurationMs;
};

#endif

