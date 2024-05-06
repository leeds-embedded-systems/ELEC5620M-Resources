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
 * Date       | Changes
 * -----------+----------------------------------
 * 27/12/2023 | Add support for Arria 10 devices.
 * 12/03/2018 | Creation of driver
 *
 */

#ifndef HPS_WATCHDOG_H_
#define HPS_WATCHDOG_H_

/*
 * Watchdog Periheral Addresses
 */

#ifdef __ARRIA10__

// There are two watchdogs. L4WD0 and L4WD1
#define HPS_WDT_L4WD0_BASE 0xFFD00200
#define HPS_WDT_L4WD1_BASE 0xFFD00300

// Set default to Watchdog 1 for Arria 10 (set in preloader device tree) if not user overridden
#ifndef HPS_WDT_L4WD_BASE
#define HPS_WDT_L4WD_BASE HPS_WDT_L4WD1_BASE
#endif

#else

// There are two watchdogs. L4WD0 and L4WD1
#define HPS_WDT_L4WD0_BASE 0xFFD02000
#define HPS_WDT_L4WD1_BASE 0xFFD03000

// Set default to Watchdog 0 for Cyclone V (set in preloader config) if not user overridden
#ifndef HPS_WDT_L4WD_BASE
#define HPS_WDT_L4WD_BASE HPS_WDT_L4WD0_BASE
#endif

#endif

#define HPS_WDT_CCVR_OFF 0x8
#define HPS_WDT_CRR_OFF  0xC

#define HPS_WDT_CRR_MAGIC 0x76

#define HPS_WDT_DECL inline __attribute__((always_inline)) static

/*
 * User APIs
 */

// Function to reset the watchdog timer.
HPS_WDT_DECL void HPS_ResetWatchdog() {
    *((volatile unsigned int *) (HPS_WDT_L4WD_BASE + HPS_WDT_CRR_OFF)) = HPS_WDT_CRR_MAGIC;
}

// Function to get value of the watchdog timer
HPS_WDT_DECL unsigned int HPS_WatchdogValue() {
    return *((volatile unsigned int *) (HPS_WDT_L4WD_BASE + HPS_WDT_CCVR_OFF));
}

#endif /* HPS_WATCHDOG_H_ */
