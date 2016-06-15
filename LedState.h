
#ifndef LED_STATE_H
#define LED_STATE_H

#include "DurationFsm.h"

class ColorLedState: public DurationState {
  public:
    // the pins to set to LOW on enter, 255 for unused colors
    ColorLedState(byte greenPin, byte redPin, byte bluePin, unsigned long durationMs, String name): DurationState(durationMs, name), greenPin(greenPin), redPin(redPin), bluePin(bluePin) {
    }
    virtual void enter() {
      reactivateLed();
    }
    virtual void exit() {
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
    }
  private:
    const byte greenPin, redPin, bluePin;
};

#endif

