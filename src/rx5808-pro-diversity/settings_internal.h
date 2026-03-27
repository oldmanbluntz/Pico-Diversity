#ifndef INTERNAL_SETTINGS_H
#define INTERNAL_SETTINGS_H


#include "settings.h"

// === EEPROM ==================================================================

// This should be incremented after every EEPROM change.
#define EEPROM_MAGIC 0x00000044

// === Receiver Modules =========================================================

#ifdef RX5808
    // rx5808 module need >20ms to tune.
    // 25 ms will do a 40 channel scan in 1 second.
    #define MIN_TUNE_TIME 25
#endif

#ifdef RX5880
    // rx5880 module needs >30ms to tune.
    // 35 ms will do a 40 channel scan in 1.4 seconds.
    #define MIN_TUNE_TIME 35
#endif

// === Display Modules =========================================================

#ifdef ADAFRUIT_ST7789_SCREENS
    #define OLED_FRAMERATE 30
#endif

#ifdef OLED_128x64_ADAFRUIT_SCREENS
    #define OLED_FRAMERATE 30
#endif

// === Misc ====================================================================

#ifdef USE_VOLTAGE_MONITORING
    #define VBAT_SMOOTH 8
    #define VBAT_PRESCALER 16
#endif

#define EEPROM_SAVE_TIME 5000

#endif // file_defined