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
 * To disable DbgPrintf logging (e.g. in production to reduce
 * code size), globally define DISABLE_VERBOSITY_DBGPRINTF.
 * This will convert the log function into a No-Op.
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 12/10/2024 | Add Debug Print Disable
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

/*
 * Verbosity level if-elseif check helpers
 */

// Level check if
// - can be used for code disabled when logging is
#ifdef DISABLE_VERBOSITY_DBGPRINTF
//     <Disable logging if debug print disabled all blocks>
#define DbgIfLevel(level) if(false)
#define DbgElse()         else if(false)
#else
#define DbgIfLevel(level) if (verbose_levelEnabled(level))
#define DbgElse()         else
#endif
// Level check else-if
// - must follow immediately after a DbgPrintf/DbgElsePrintf/DbgIfLevel/DbgElseIfLevel line.
#define DbgElseIfLevel(level) else DbgIfLevel(level)

// Printf with level check
#define DbgPrintf(level, ...) DbgIfLevel(level) printf(__VA_ARGS__)
// Chained printf with level check
// - must follow immediately after a DbgPrintf/DbgElsePrintf/DbgIfLevel/DbgElseIfLevel line.
#define DbgElsePrintf(level, ...) DbgElseIfLevel(level) printf(__VA_ARGS__)


#endif /* VERBOSITY_H_ */
