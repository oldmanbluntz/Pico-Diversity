#ifndef CHANNELS_H
#define CHANNELS_H

#include "settings.h"

// Force the size to 72 for the 9 standard bands
#define CHANNELS_SIZE 72

namespace Channels {
    const uint16_t getSynthRegisterB(uint8_t index);
    const uint16_t getFrequency(uint8_t index);
    const char *getName(uint8_t index);
    const uint8_t getOrderedIndex(uint8_t index);
    const uint8_t getOrderedIndexFromIndex(uint8_t index);
}

#endif