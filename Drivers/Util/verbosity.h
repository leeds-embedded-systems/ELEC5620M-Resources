/*
 * Debug Verbosity Levels
 * ----------------------
 *
 * Provides routines for controlling the verbosity level of
 * log messages, as well as providing a `DbgPrintf` wrapper
 * which will only call printf if any of the specified level
 * mask bits are enabled.
 *
 * For example `DbgPrintf(VERBOSE_INFO, "My Message %d", 1)`
 * will only log the message if the info flag is enabled in
 * the verbosity mask.
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 29/12/2023 | Creation of driver
 *
 */

#ifndef VERBOSITY_H_
#define VERBOSITY_H_

#include <stdbool.h>
#include <stdio.h>

// Verbosity Enum
#ifndef VERBOSITY_LEVEL_MASKS_DEFINED
#define VERBOSITY_LEVEL_MASKS_DEFINED
// Verbosity masks enum must not be changed
// as it is used in multiple systems which
// may have their own implementation.

#include "Util/bit_helpers.h"

typedef enum {
    VERBOSE_ERROR     = _BV(0),
    VERBOSE_WARNING   = _BV(1),
    VERBOSE_INFO      = _BV(2),
    VERBOSE_EXTRAINFO = _BV(3),
    VERBOSE_DISABLED  = (0), //off
    VERBOSE_LEVEL0    = (0), //off
    VERBOSE_LEVEL1    = (VERBOSE_ERROR),
    VERBOSE_LEVEL2    = (VERBOSE_ERROR | VERBOSE_WARNING),
    VERBOSE_LEVEL3    = (VERBOSE_ERROR | VERBOSE_WARNING | VERBOSE_INFO),
    VERBOSE_LEVEL4    = (VERBOSE_ERROR | VERBOSE_WARNING | VERBOSE_INFO | VERBOSE_EXTRAINFO)
} VerbosityLevelMasks;

#endif

// Sets the full verbosity mask
void verbose_setMask(VerbosityLevelMasks mask);

// Sets a specific level (bit-wise OR with mask)
void verbose_enableLevel(VerbosityLevelMasks mask);

// Clear a specific level (bit-wise AND with inverse mask)
void verbose_disableLevel(VerbosityLevelMasks mask);

// Get full verbosity mask
VerbosityLevelMasks verbose_getMask();

// Check if a specific level is enabled (returns true if any bit is mask is set)
bool verbose_levelEnabled(VerbosityLevelMasks mask);

// Printf with level check
#define DbgPrintf(level, ...) if (verbose_levelEnabled(level)) printf(__VA_ARGS__)

#endif /* VERBOSITY_H_ */
