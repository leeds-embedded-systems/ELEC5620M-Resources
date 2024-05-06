/*
 * Generic Watchdog Routines
 * -------------------------
 *
 * Maps APIs specific to different CPU types to a
 * common set of APIs.
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

#ifndef UTIL_WATCHDOG_H
#define UTIL_WATCHDOG_H

#include <stdbool.h>
#include <stdint.h>

#include "Util/error.h"

#if defined(__arm__)
// ARM uses HPS Watchdog
#include "HPS_Watchdog/HPS_Watchdog.h"

// Reset the Watchdog Timer
#define ResetWDT() HPS_ResetWatchdog()

#else

// No watchdog. Do nothing.
#define ResetWDT() ((void)0)

#endif

#endif /* UTIL_WATCHDOG_H */
