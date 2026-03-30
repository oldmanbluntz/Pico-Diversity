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
    memcpy(this, &EepromDefaults, sizeof(EepromDefaults));
    
    // Format all 8 models to default minimums/empty names
    for(int i = 0; i < 8; i++) {
        this->models[i].name[0] = '\0';
        this->models[i].rssiAMin = RSSI_MIN_VAL;
        this->models[i].rssiAMax = RSSI_MAX_VAL;
        #ifdef USE_DIVERSITY
            this->models[i].rssiBMin = RSSI_MIN_VAL;
            this->models[i].rssiBMax = RSSI_MAX_VAL;
        #endif
    }
    
    // Create a default Model 1
    strcpy(this->models[0].name, "Model 1");
    this->activeModel = 0;

    this->save();
}