#include <Arduino.h>
#include "timer.h"


// PICO PORT: Upgraded delay to uint32_t.
// This prevents overflow for timers longer than 65 seconds (uint16 limit).
Timer::Timer(uint32_t delay) {
    this->delay = delay;
    this->nextTick = millis() + this->delay;
    this->ticked = false;
}

const bool Timer::hasTicked() {
    if (this->ticked)
        return true;

    if (millis() >= this->nextTick) {
        this->ticked = true;
        return true;
    }

    return false;
}

void Timer::reset() {
    this->nextTick = millis() + this->delay;
    this->ticked = false;
}