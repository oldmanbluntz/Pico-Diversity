#include <string.h>
#include <EEPROM.h>
#include <Arduino.h>

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
    // PICO FIX: Ensure the 512 byte buffer is pulled from Flash into RAM
    // EEPROM.begin(512) must have been called in main.cpp first.
    EEPROM.get(0, *this);

    if (this->magic != EEPROM_MAGIC)
        this->initDefaults();
}

void EepromSettings::save() {
    EEPROM.put(0, *this);
    
    // PICO FIX: Mandatory commit call. 
    // RP2040 writes to a RAM buffer; this line pushes it to physical Flash.
    EEPROM.commit(); 
}

void EepromSettings::markDirty() {
    isDirty = true;
}

void EepromSettings::initDefaults() {
    // PICO FIX: Replaced AVR-specific memcpy_P with standard memcpy.
    // The Pico has a unified memory map where Flash is directly accessible.
    memcpy(this, &EepromDefaults, sizeof(EepromDefaults));
    this->save();
}