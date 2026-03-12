#include <Arduino.h>
#include "channels.h"
#include "settings.h"

// Channels to send to the SPI registers
static const uint16_t channelTable[] = {
    #define _CHANNEL_REG_FLO(f) ((f - 479) / 2)
    #define _CHANNEL_REG_N(f) (_CHANNEL_REG_FLO(f) / 32)
    #define _CHANNEL_REG_A(f) (_CHANNEL_REG_FLO(f) % 32)
    #define CHANNEL_REG(f) (_CHANNEL_REG_N(f) << 7) | _CHANNEL_REG_A(f)

    // A
    CHANNEL_REG(5865), CHANNEL_REG(5845), CHANNEL_REG(5825), CHANNEL_REG(5805),
    CHANNEL_REG(5785), CHANNEL_REG(5765), CHANNEL_REG(5745), CHANNEL_REG(5725),
    // B
    CHANNEL_REG(5733), CHANNEL_REG(5752), CHANNEL_REG(5771), CHANNEL_REG(5790),
    CHANNEL_REG(5809), CHANNEL_REG(5828), CHANNEL_REG(5847), CHANNEL_REG(5866),
    // E
    CHANNEL_REG(5705), CHANNEL_REG(5685), CHANNEL_REG(5665), CHANNEL_REG(5645),
    CHANNEL_REG(5885), CHANNEL_REG(5905), CHANNEL_REG(5925), CHANNEL_REG(5945),
    // F
    CHANNEL_REG(5740), CHANNEL_REG(5760), CHANNEL_REG(5780), CHANNEL_REG(5800),
    CHANNEL_REG(5820), CHANNEL_REG(5840), CHANNEL_REG(5860), CHANNEL_REG(5880),
    // R
    CHANNEL_REG(5658), CHANNEL_REG(5695), CHANNEL_REG(5732), CHANNEL_REG(5769),
    CHANNEL_REG(5806), CHANNEL_REG(5843), CHANNEL_REG(5880), CHANNEL_REG(5917),
    // U
    CHANNEL_REG(5325), CHANNEL_REG(5348), CHANNEL_REG(5366), CHANNEL_REG(5384),
    CHANNEL_REG(5402), CHANNEL_REG(5420), CHANNEL_REG(5438), CHANNEL_REG(5456),
    // O
    CHANNEL_REG(5474), CHANNEL_REG(5492), CHANNEL_REG(5510), CHANNEL_REG(5528),
    CHANNEL_REG(5546), CHANNEL_REG(5564), CHANNEL_REG(5582), CHANNEL_REG(5600),
    // L
    CHANNEL_REG(5333), CHANNEL_REG(5373), CHANNEL_REG(5413), CHANNEL_REG(5453),
    CHANNEL_REG(5493), CHANNEL_REG(5533), CHANNEL_REG(5573), CHANNEL_REG(5613),
    // H
    CHANNEL_REG(5653), CHANNEL_REG(5693), CHANNEL_REG(5733), CHANNEL_REG(5773),
    CHANNEL_REG(5813), CHANNEL_REG(5853), CHANNEL_REG(5893), CHANNEL_REG(5933)

    #undef _CHANNEL_REG_FLO
    #undef _CHANNEL_REG_A
    #undef _CHANNEL_REG_N
    #undef CHANNEL_REG
};

// Channels with their Mhz Values
static const uint16_t channelFreqTable[] = {
    5865, 5845, 5825, 5805, 5785, 5765, 5745, 5725, // A
    5733, 5752, 5771, 5790, 5809, 5828, 5847, 5866, // B
    5705, 5685, 5665, 5645, 5885, 5905, 5925, 5945, // E
    5740, 5760, 5780, 5800, 5820, 5840, 5860, 5880, // F
    5658, 5695, 5732, 5769, 5806, 5843, 5880, 5917, // R
    5325, 5348, 5366, 5384, 5402, 5420, 5438, 5456, // U
    5474, 5492, 5510, 5528, 5546, 5564, 5582, 5600, // O
    5333, 5373, 5413, 5453, 5493, 5533, 5573, 5613, // L
    5653, 5693, 5733, 5773, 5813, 5853, 5893, 5933  // H
};

// Encode channel names as an 8-bit value
static const uint8_t channelNames[] = {
    #define _CHANNEL_NAMES(l) (uint8_t) ((l - 65) << 3)
    #define CHANNEL_NAMES(l) \
        _CHANNEL_NAMES(l) | 0, _CHANNEL_NAMES(l) | 1, _CHANNEL_NAMES(l) | 2, _CHANNEL_NAMES(l) | 3, \
        _CHANNEL_NAMES(l) | 4, _CHANNEL_NAMES(l) | 5, _CHANNEL_NAMES(l) | 6, _CHANNEL_NAMES(l) | 7

    CHANNEL_NAMES('A'),
    CHANNEL_NAMES('B'),
    CHANNEL_NAMES('E'),
    CHANNEL_NAMES('F'),
    CHANNEL_NAMES('R'), 
    CHANNEL_NAMES('U'),
    CHANNEL_NAMES('O'),
    CHANNEL_NAMES('L'),
    CHANNEL_NAMES('H')

    #undef CHANNEL_NAMES
    #undef _CHANNEL_NAMES
};

// All Channels ordered by Mhz for the spectrum scanner
static const uint8_t channelFreqOrderedIndex[] = {
    40, 56, 41, 42, 57, 43, 44, 58, 45, 46, 59, 47, 48, 49, 60, 50, 51, 61, 52, 53, 62, 54, 55, 63,
    19, 64, 32, 18, 17, 65, 33, 16,  7, 34,  8, 66, 24,  6,  9, 25,  5, 35, 10, 67, 26,  4, 11, 27,
     3, 36, 12, 68, 28,  2, 13, 29, 37,  1, 14, 69, 30,  0, 15, 31, 38, 20, 70, 21, 39, 22, 71, 23
};

static const uint8_t channelIndexToOrderedIndex[] = {
    61, 57, 53, 48, 45, 40, 37, 32, 34, 38, 42, 46, 50, 54, 58, 62, 31, 28, 27, 24, 65, 67, 69, 71, 
    36, 39, 44, 47, 52, 55, 60, 63, 26, 30, 33, 41, 49, 56, 64, 68,  0,  2,  3,  5,  6,  8,  9, 11, 
    12, 13, 15, 16, 18, 19, 21, 22,  1,  4,  7, 10, 14, 17, 20, 23, 25, 29, 35, 43, 51, 59, 66, 70
};

namespace Channels {
    const uint16_t getSynthRegisterB(uint8_t index) {
        return channelTable[index];
    }

    const uint16_t getFrequency(uint8_t index) {
        return channelFreqTable[index];
    }

    char nameBuffer[3];
    const char *getName(uint8_t index) {
        uint8_t encodedName = channelNames[index];

        nameBuffer[0] = 65 + (encodedName >> 3);
        nameBuffer[1] = 48 + (encodedName & (255 >> (8 - 3))) + 1;
        nameBuffer[2] = '\0';

        return nameBuffer;
    }

    const uint8_t getOrderedIndex(uint8_t index) {
        return channelFreqOrderedIndex[index];
    }

    const uint8_t getOrderedIndexFromIndex(uint8_t index) {
        return channelIndexToOrderedIndex[index];
    }
}