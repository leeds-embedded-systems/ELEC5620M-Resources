/*
 * HPS Watchdog Reset
 * ------------------------------
 * Description: 
 * Simple inline functions for resetting watchdog and
 * returning the current watchdog timer value.
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 */

#ifndef HPS_WATCHDOG_H_
#define HPS_WATCHDOG_H_

//#define for backwards compatibility
#define ResetWDT() HPS_ResetWatchdog()

// Function to reset the watchdog timer.
__forceinline static void HPS_ResetWatchdog() {
    *((volatile unsigned int *) 0xFFD0200C) = 0x76;
}

// Function to get value of the watchdog timer
__forceinline static unsigned int HPS_WatchdogValue() {
    return *((volatile unsigned int *) 0xFFD02008);
}

#endif /* HPS_WATCHDOG_H_ */
