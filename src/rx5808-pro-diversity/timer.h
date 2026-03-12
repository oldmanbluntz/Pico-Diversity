#ifndef TIMER_H
#define TIMER_H


#include <stdint.h>


class Timer {
    private:
        uint32_t nextTick;
        uint32_t delay; // PICO PORT: Changed from uint16_t to uint32_t
        bool ticked;

    public:
        Timer(uint32_t delay);
        const bool hasTicked();
        void reset();
};


#endif