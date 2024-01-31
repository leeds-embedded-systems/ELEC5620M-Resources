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

#include "verbosity.h"

// Current verbose mode
// - Default depends on whether debug or release
static VerbosityLevelMasks _verboseMode =
#ifdef DEBUG
    VERBOSE_LEVEL3
#else
    VERBOSE_LEVEL1
#endif
;


// Sets the full verbosity mask
void verbose_setMask(VerbosityLevelMasks mask) {
    _verboseMode = mask;
}

// Sets a specific level (bit-wise OR with mask)
void verbose_enableLevel(VerbosityLevelMasks mask) {
    _verboseMode |= mask;
}

// Clear a specific level (bit-wise AND with inverse mask)
void verbose_disableLevel(VerbosityLevelMasks mask) {
    _verboseMode &= ~mask;
}

// Get full verbosity mask
VerbosityLevelMasks verbose_getMask() {
    return _verboseMode;
}

// Check if a specific level is enabled (returns true if any bit is mask is set)
bool verbose_levelEnabled(VerbosityLevelMasks mask) {
    return !!(_verboseMode & mask);
}

