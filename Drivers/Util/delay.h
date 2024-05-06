/*
 * Generic Delay Routines
 * -------------------------
 *
 * Maps APIs specific to different CPU types to a
 * common set of APIs.
 * 
 * Provides usleep() API header include.
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 09/04/2024 | Creation of header
 * 
 */

#ifndef UTIL_DELAY_H
#define UTIL_DELAY_H

#include <stdbool.h>
#include <stdint.h>

#include "Util/error.h"

#if defined(__arm__)

// ARM HPS requires custom usleep implementation
#include "HPS_usleep/HPS_usleep.h"

#else

// Otherwise use usleep definition from unistd.h
#include <unistd.h>

#endif

// Simple NOP Delay
static inline void nopDelay(unsigned int loopCount) {
    for (unsigned int nopIdx = 0; nopIdx < loopCount; nopIdx++) {
        asm volatile("nop");
    }
}

#endif /* UTIL_DELAY_H */
