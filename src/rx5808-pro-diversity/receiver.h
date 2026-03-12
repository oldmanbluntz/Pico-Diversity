#ifndef RECEIVER_H
#define RECEIVER_H


#include <stdint.h>
#include "settings.h"


#define RECEIVER_LAST_DELAY 50
#define RECEIVER_LAST_DATA_SIZE 24


namespace Receiver {
    enum class ReceiverId : uint8_t {
        A
        #ifdef USE_DIVERSITY
            ,
            B
        #endif
    };

    #ifdef USE_DIVERSITY
        enum class DiversityMode : uint8_t {
            AUTO,
            FORCE_A,
            FORCE_B
        };
    #endif


    extern ReceiverId activeReceiver;
    extern uint8_t activeChannel;

    extern uint8_t rssiA;
    extern uint16_t rssiARaw;
    extern uint8_t rssiALast[RECEIVER_LAST_DATA_SIZE];
    #ifdef USE_DIVERSITY
        extern uint8_t rssiB;
        extern uint16_t rssiBRaw;
        extern uint8_t rssiBLast[RECEIVER_LAST_DATA_SIZE];
    #endif

    void setChannel(uint8_t channel);
    uint16_t updateRssi();
    void setActiveReceiver(ReceiverId receiver = ReceiverId::A);
    
    #ifdef USE_DIVERSITY
        // PICO FIX: Updated prototype to match .cpp (was uint8_t, now DiversityMode)
        void setDiversityMode(DiversityMode mode);
        void switchDiversity();
    #endif

    bool isRssiStable();


    void setup();
    void update();
}


#endif