/*
 * SPI driver based on fs_skyrf_58g-main.c Written by Simon Chambers
 ... [Existing Header Comments Preserved] ...
*/

#include <arduino.h>
#include <EEPROM.h> 
#include <Wire.h> // PICO FIX: Required for manual pin mapping

#include "settings.h"
#include "settings_internal.h"
#include "settings_eeprom.h"

#include "channels.h"
#include "receiver.h"
#include "receiver_spi.h"
#include "buttons.h"
#include "state.h"

#include "ui.h"

void setupPins();
void setupSettings();

static void globalMenuButtonHandler(
    Button button,
    Buttons::PressType pressType
);


void setup()
{
    // PICO FIX: Explicitly assign I2C pins BEFORE Wire.begin() or Ui::setup().
    // This prevents the Pico from hanging on the default GP4/GP5 pins 
    // where your buttons are physically located.
    delay(500);
    Wire.setSDA(PIN_OLED_SDA); 
    Wire.setSCL(PIN_OLED_SCL);
    Wire.begin();

    // PICO FIX: Initialize the Flash Emulation Buffer (512 bytes).
    // MUST be called before setupSettings() or any EEPROM access.
    EEPROM.begin(512);

    setupPins();

    // Enable buzzer and LED for duration of setup process.
    digitalWrite(PIN_LED, HIGH);
    digitalWrite(PIN_BUZZER, LOW);

    setupSettings();

    StateMachine::setup();
    Receiver::setup();
    delay(2000);
    Ui::setup();

    for(int i=0; i<10; i++) { // 10 fast blinks = Setup reached the end
        digitalWrite(PIN_LED, HIGH); delay(50);
        digitalWrite(PIN_LED, LOW); delay(50);
    }

    Receiver::setActiveReceiver(Receiver::ReceiverId::B);

    #ifdef USE_IR_EMITTER
        Serial.begin(9600);
    #endif
    #ifdef USE_SERIAL_OUT
        Serial.begin(250000);
    #endif

    // Setup complete.
    digitalWrite(PIN_LED, LOW);
    digitalWrite(PIN_BUZZER, HIGH);

    Buttons::registerChangeFunc(globalMenuButtonHandler);

    // Switch to initial state.
    StateMachine::switchState(StateMachine::State::SEARCH);
}

void setupPins() {
    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_BUZZER, OUTPUT);
    pinMode(PIN_BUTTON_UP, INPUT_PULLUP);
    pinMode(PIN_BUTTON_MODE, INPUT_PULLUP);
    pinMode(PIN_BUTTON_DOWN, INPUT_PULLUP);
    
    // PICO FIX: Guard SAVE button if not used
    #ifdef PIN_BUTTON_SAVE
        pinMode(PIN_BUTTON_SAVE, INPUT_PULLUP);
    #endif

    pinMode(PIN_LED_A, OUTPUT);
    
    #ifdef USE_DIVERSITY
        pinMode(PIN_LED_B, OUTPUT);
    #endif

    // PICO FIX: Changed INPUT_PULLUP to INPUT for RSSI.
    pinMode(PIN_RSSI_A, INPUT);
    
    #ifdef USE_DIVERSITY
        pinMode(PIN_RSSI_B, INPUT);
    #endif

    // --- SPI Initialization ---
    pinMode(PIN_SPI_SLAVE_SELECT, OUTPUT);
    pinMode(PIN_SPI_DATA, OUTPUT);
    pinMode(PIN_SPI_CLOCK, OUTPUT);

    digitalWrite(PIN_SPI_SLAVE_SELECT, HIGH); 
    
    #ifdef USE_DIVERSITY
        #ifdef PIN_SLAVE_SELECT_B
            pinMode(PIN_SLAVE_SELECT_B, OUTPUT);
            digitalWrite(PIN_SLAVE_SELECT_B, HIGH);
        #endif
    #endif

    digitalWrite(PIN_SPI_CLOCK, LOW);
    digitalWrite(PIN_SPI_DATA, LOW);
}

void setupSettings() {
    EepromSettings.load();
    Receiver::setChannel(EepromSettings.startChannel);
}


void loop() {
    Receiver::update();
    Buttons::update();
    StateMachine::update();
    Ui::update();
    EepromSettings.update();

    if (
        StateMachine::currentState != StateMachine::State::SCREENSAVER
        && StateMachine::currentState != StateMachine::State::BANDSCAN
        && (millis() - Buttons::lastChangeTime) >
            (SCREENSAVER_TIMEOUT * 1000)
    ) {
        StateMachine::switchState(StateMachine::State::SCREENSAVER);
    }
}


static void globalMenuButtonHandler(
    Button button,
    Buttons::PressType pressType
) {
    if (
        StateMachine::currentState != StateMachine::State::MENU &&
        button == Button::MODE &&
        pressType == Buttons::PressType::HOLDING
    ) {
        StateMachine::switchState(StateMachine::State::MENU);
    }
}