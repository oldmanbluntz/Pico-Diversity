#ifndef SETTINGS_H
#define SETTINGS_H
#include <Arduino.h> 


// === Display Module ==========================================================
//
// You can choose one display module only. Only 128x64 OLED displays are
// supported.
//
// =============================================================================

// SSH1106 needs https://github.com/badzz/Adafruit_SH1106 installed.
//#define SH1106

//#define TVOUT_SCREENS
#define OLED_128x64_ADAFRUIT_SCREENS

// Enable this if your screen is upside down.
//#define USE_FLIP_SCREEN

#ifdef OLED_128x64_ADAFRUIT_SCREENS
    #define OLED_ADDRESS 0x3C // I2C address for display (0x3C or 0x3D, usually)
#endif

// === Receiver Module =========================================================
//
// Select which receiever you are using. Required for time tuning.
//
// =============================================================================

#define RX5808
//#define RX5880

// Can enable this to powerdown the audio blocks on the RX58xx if you don't
// need it. Save a tiny bit of power, make your videos less noisy.
//
// WARNING: Makes RSSI act a little weird.
//#define DISABLE_AUDIO

// === Features ===============================================================
//
// Keep in mind that there is limited program memory. Only enable the features
// you need.
//
// =============================================================================

#define USE_DIVERSITY

// PICO FIX: DISABLED "Fast Switching".
// This uses AVR-specific Port Registers (PORTB, DDRB) which do not exist on Pico.
// The Pico is 133MHz (vs Arduino 16MHz), so standard switching is plenty fast.
// #define USE_DIVERSITY_FAST_SWITCHING

//#define USE_IR_EMITTER
//#define USE_SERIAL_OUT // Not compatible with IR emitter.

// You can use any of the arduino analog pins to measure the voltage of the
// battery. See additional configuration below.
//#define USE_VOLTAGE_MONITORING

// Choose if you wish to use 8 additional channels:
//     5362 MHz
//     5399 MHz
//     5436 MHz
//     5473 MHz
//     5510 MHz
//     5547 MHz
//     5584 MHz
//     5621 MHz
// Local laws may prohibit the use of these frequencies so use at your own risk!
#define USE_LBAND

// === Pins (Raspberry Pi Pico Mapping) ========================================

// --- BUTTONS ---
#define PIN_BUTTON_UP     2   // GP2
#define PIN_BUTTON_MODE   3   // GP3
#define PIN_BUTTON_DOWN   4   // GP4
#define PIN_BUTTON_SAVE   5   // GP5 (Optional)

// --- SYSTEM INDICATORS ---
#ifdef PIN_LED
    #undef PIN_LED
#endif
#define PIN_LED           22  // GP22 (Status LED)

#define PIN_BUZZER        16  // GP16 (Passive Buzzer)

// --- SPI / TUNING CONTROL (Bit-Banged) ---
#define PIN_SPI_DATA      19  // GP19 (MOSI)
#define PIN_SPI_CLOCK     18  // GP18 (SCK)
#define PIN_SPI_SLAVE_SELECT 20 // GP20 (RX A Latch)
#define PIN_SLAVE_SELECT_B   21 // GP21 (RX B Latch)

// --- DIVERSITY & VIDEO SWITCHING ---
#define PIN_RSSI_A        26  // GP26 (ADC0) - Analog Input

// Map "LEDs" to the ADG6412 Switch Pins (Control Logic)
#define PIN_LED_A         14  // GP14 (RX A Active / Video Switch A)

#define USE_DIVERSITY         // Force Diversity Mode ON
#ifdef USE_DIVERSITY
    #define PIN_RSSI_B    27  // GP27 (ADC1) - Analog Input
    #define PIN_LED_B     15  // GP15 (RX B Active / Video Switch B)
#endif

// --- DISPLAY (I2C) ---
// On Pico, default I2C0 is GP0/GP1, but you can define others here.
// You must call Wire.setSDA/SCL in setup() if changing these from defaults.
#define PIN_OLED_SDA      0   // GP0
#define PIN_OLED_SCL      1   // GP1


#ifdef USE_VOLTAGE_MONITORING
    #define PIN_VBAT      28  // GP28 (ADC2) - Example for Pico
#endif

// === Diversity ===============================================================

#ifdef USE_DIVERSITY
    // RSSI strength should be greater than the value below (percent) over the
    // other receiver before we switch. This pervents flicker when RSSI values
    // are close and delays diversity checks counter.
    #define DIVERSITY_HYSTERESIS 2

    // How long (ms) the RSSI strength has to have a greater difference than the
    // above before switching.
    #define DIVERSITY_HYSTERESIS_PERIOD 5
#endif

// === Voltage Monitoring ======================================================

#ifdef USE_VOLTAGE_MONITORING
    // You can use any Arduino analog input to measure battery voltage. Keep in
    // mind that A4 and A5 is used by OLED and A6 and A7 are used for measuring
    // RSSI.
    //
    // Use a voltage divider to lower the voltage to max 5V - values for max 13V
    // (3s). You can use a 100nF capacitor near the Arduino pin to smooth the
    // voltage.
    //
    //           R1 = 5.6k
    //    BAT+ ----====----+----+---- ARDUINO ANALOG PIN
    //                     |    |
    //                     |    |  (optional)
    //                     |    || 100n CAP
    //                     |    |
    //           R2 = 3.3k |    |
    //    BAT- ----====----|----|

    // Voltage levels
    #define WARNING_VOLTAGE 108 // 3.6V per cell for 3S
    #define CRITICAL_VOLTAGE 100 // 3.3V per cell for 3S
    #define VBAT_SCALE 119
    #define VBAT_OFFSET 0

    // Alarm sounds
    #define ALARM_EVERY_MSEC 5000
    #define CRITICAL_BEEP_EVERY_MSEC 400
    #define CRITICAL_BEEPS 3
    #define WARNING_BEEP_EVERY_MSEC 200
    #define WARNING_BEEPS 2
#endif

// === RSSI ====================================================================

// RSSI default raw range.
#define RSSI_MIN_VAL 100
#define RSSI_MAX_VAL 350

// 75% threshold, when channel is printed in spectrum.
#define RSSI_SEEK_FOUND 75

// 80% under max value for RSSI.
#define RSSI_SEEK_TRESHOLD 80

// Scan loops for setup run.
#define RSSI_SETUP_RUN 3

// === Misc ====================================================================

// Key debounce delay in milliseconds.
// Good values are in the range of 100-200ms.
// Shorter values will make it more reactive, but may lead to double trigger.
#define BUTTON_DEBOUNCE_DELAY 100

#define SCREENSAVER_TIMEOUT 30 // Seconds to wait before entering screensaver
#define SCREENSAVER_DISPLAY_CYCLE 3 // Seconds between switching logo/channel

// Time needed to hold mode to get to menu
#define BUTTON_WAIT_FOR_MENU 1000

#endif // file_defined