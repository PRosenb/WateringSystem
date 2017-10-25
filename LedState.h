
#ifndef LED_STATE_H
#define LED_STATE_H

#include "DurationFsm.h"

class ColorLedState: public DurationState, public Runnable {
  public:
    // the pins to set to LOW on enter, 255 for unused colors
    ColorLedState(byte greenValue, byte redValue, byte blueValue, unsigned long ledOnDurationMs, String name)
      : DurationState(INFINITE_DURATION, name), greenValue(greenValue), redValue(redValue), blueValue(blueValue), ledOnDurationMs(ledOnDurationMs) {
    }
    ColorLedState(byte greenValue, byte redValue, byte blueValue, unsigned long ledOnDurationMs, String name, SuperState * const superState)
      : DurationState(INFINITE_DURATION, name, superState), greenValue(greenValue), redValue(redValue), blueValue(blueValue), ledOnDurationMs(ledOnDurationMs) {
    }
    virtual void enter() {
      reactivateLed();
    }
    virtual void exit() {
      deactivateLed();
      scheduler.removeCallbacks(this);
    }
    void reactivateLed() {
#ifndef COLOR_LED_INVERTED
      analogWrite(COLOR_LED_GREEN_PIN, greenValue);
      analogWrite(COLOR_LED_RED_PIN, redValue);
      analogWrite(COLOR_LED_BLUE_PIN, blueValue);
#else
      analogWrite(COLOR_LED_GREEN_PIN, 255 - greenValue);
      analogWrite(COLOR_LED_RED_PIN, 255 - redValue);
      analogWrite(COLOR_LED_BLUE_PIN, 255 - blueValue);
#endif
      if (ledOnDurationMs != INFINITE_DURATION) {
        scheduler.scheduleDelayed(this, ledOnDurationMs);
      }
    }
    void deactivateLed() {
#ifndef COLOR_LED_INVERTED
      analogWrite(COLOR_LED_GREEN_PIN, 0);
      analogWrite(COLOR_LED_RED_PIN, 0);
      analogWrite(COLOR_LED_BLUE_PIN, 0);
#else
      analogWrite(COLOR_LED_GREEN_PIN, 255);
      analogWrite(COLOR_LED_RED_PIN, 255);
      analogWrite(COLOR_LED_BLUE_PIN, 255);
#endif
    }
    void run() {
      deactivateLed();
    }
    bool isActive() {
      return ledOnDurationMs == INFINITE_DURATION || scheduler.isScheduled(this);
    }
  private:
    const byte greenValue, redValue, blueValue;
    const unsigned long ledOnDurationMs;
};

#endif

