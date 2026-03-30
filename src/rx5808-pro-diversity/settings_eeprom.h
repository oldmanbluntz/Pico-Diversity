#ifndef EEPROM_SETTINGS_H
#define EEPROM_SETTINGS_H

#include <stdint.h>

#include "settings.h"
#include "settings_internal.h"
#include "receiver.h"

struct ModelData {
    char name[9];
    uint16_t rssiAMin;
    uint16_t rssiAMax;
    #ifdef USE_DIVERSITY
        uint16_t rssiBMin;
        uint16_t rssiBMax;
    #endif
};

struct EepromSettings {
    uint32_t magic;
    uint8_t startChannel;
    uint8_t beepEnabled;
    uint8_t searchManual;
    uint8_t searchOrderByChannel;

    // These represent the currently loaded/active system values
    uint16_t rssiAMin;
    uint16_t rssiAMax;

    // Customization Settings
    uint8_t uiLayout;
    uint8_t uiScheme;
    uint8_t screensaverStyle;

    // Model Storage
    uint8_t activeModel;
    ModelData models[8];

    #ifdef USE_DIVERSITY
        Receiver::DiversityMode diversityMode;
        uint16_t rssiBMin;
        uint16_t rssiBMax;
    #endif

    #ifdef USE_VOLTAGE_MONITORING
        uint8_t vbatScale;
        uint8_t vbatWarning;
        uint8_t vbatCritical;
    #endif

    void update();
    void load();
    void save();
    void markDirty();
    void initDefaults();
};

static const struct {
    uint32_t magic = EEPROM_MAGIC;
    uint8_t startChannel = 0;
    uint8_t beepEnabled = true;
    uint8_t searchManual = false;
    uint8_t searchOrderByChannel = false;
    uint16_t rssiAMin = RSSI_MIN_VAL;
    uint16_t rssiAMax = RSSI_MAX_VAL;

    // Customization Defaults
    uint8_t uiLayout = DEFAULT_UI_LAYOUT;
    uint8_t uiScheme = DEFAULT_UI_SCHEME;
    uint8_t screensaverStyle = DEFAULT_SCREENSAVER_STYLE;

    // Model Defaults
    uint8_t activeModel = 0;
    ModelData models[8] = {}; 

    #ifdef USE_DIVERSITY
        Receiver::DiversityMode diversityMode = Receiver::DiversityMode::AUTO;
        uint16_t rssiBMin = RSSI_MIN_VAL;
        uint16_t rssiBMax = RSSI_MAX_VAL;
    #endif

    #ifdef USE_VOLTAGE_MONITORING
        uint8_t vbatScale = VBAT_SCALE;
        uint8_t vbatWarning = WARNING_VOLTAGE;
        uint8_t vbatCritical = CRITICAL_VOLTAGE;
    #endif
} EepromDefaults;

extern EepromSettings EepromSettings;

#endif