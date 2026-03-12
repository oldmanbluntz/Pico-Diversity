#include "pstr_helper.h"

// ============================================================================
// PICO PORT NOTE:
// The Raspberry Pi Pico (ARM Cortex-M0+) allows direct access to Flash memory.
// The original AVR "Program Space" buffer logic is not required here.
//
// This file is intentionally left (mostly) empty to satisfy the build system.
// ============================================================================

// If legacy code explicitly asks for the buffer (unlikely), we define it here.
char PSTR2_BUFFER[PSTR2_BUFFER_SIZE]; 