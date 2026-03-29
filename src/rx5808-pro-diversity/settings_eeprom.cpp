#include <string.h>
#include <EEPROM.h>
#include <Arduino.h> // Added for standard definitions

#include "settings.h"
#include "settings_internal.h"
#include "settings_eeprom.h"

#include "timer.h"


static Timer saveTimer = Timer(EEPROM_SAVE_TIME);
static bool isDirty = false;


struct EepromSettings EepromSettings;


void EepromSettings::update() {
    if (isDirty) {
        if (saveTimer.hasTicked()) {
            isDirty = false;
            saveTimer.reset();

            this->save();
        }
    }
}

void EepromSettings::load() {
    // PICO NOTE: EEPROM.begin(512) was added to the main .ino setup().
    // This reads the emulated EEPROM from Flash into RAM.
    EEPROM.get(0, *this);

    if (this->magic != EEPROM_MAGIC)
        this->initDefaults();
}

void EepromSettings::save() {
    EEPROM.put(0, *this);
    
    // PICO FIX: Explicitly commit changes to Flash.
    // The Pico uses a RAM buffer for EEPROM emulation. 
    // Without commit(), changes are lost on power cycle.
    EEPROM.commit(); 
}

void EepromSettings::markDirty() {
    isDirty = true;
}


void EepromSettings::initDefaults() {
    // PICO FIX: Replaced memcpy_P (AVR specific) with standard memcpy.
    // The Pico has a unified memory map; Flash is directly readable.
    memcpy(this, &EepromDefaults, sizeof(EepromDefaults));
    this->save();
}