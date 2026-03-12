#ifndef PSTR_HELPER_H
#define PSTR_HELPER_H

// PICO PORT: Removed <avr/pgmspace.h>

// On AVR, PSTR() puts a string in Flash. 
// On Pico, strings are in Flash by default, but accessible directly.
#ifndef PSTR
    #define PSTR(x) x
#endif

// On AVR, PSTR2 copied the Flash string to a RAM buffer.
// On Pico, we just pass the pointer through directly. 
// No buffer copy needed.
#define PSTR2(x) x

// Defined to prevent "undefined reference" errors if legacy code checks the size.
#define PSTR2_BUFFER_SIZE 48 

#endif