/*
 * HPS Timer Based "usleep"
 * ------------------------------
 *
 * The POSIX usleep() function does not exist for bare
 * metal applications with armcc/armclang. This library
 * adds a similar function which allows the processor to
 * be stalled for `x` microseconds.
 *
 * Delays up to ~2.09 seconds are supported. This is to
 * ensure the watchdog won't time out before the delay is
 * finished. This corresponds to a value of 0x200000 us.
 * The watchdog will be reset just before the delay starts
 * and just after it finishes.
 *
 * By default, the delay is performed using the HPS SP1
 * Timer at base. This is a hardware timer in the SoC HPS
 * bridge, so does not conflict with the ARM A9 private
 * timer module. The timer can be changed to any of the
 * four HPS timers by calling the optional `selectTimer()`
 * function and providing a base address. The default
 * timer can also be overridden by globally defining the
 * DEFAULT_USLEEP_TIMER_BASE macro to point to the desired
 * default timer.
 *
 * The default timer frequency can be overridden by globally
 * defining DEFAULT_USLEEP_TIMER_FREQ. The default if not
 * defined is 100MHz (Cyclone V) or 125MHz (Arria 10) which
 * is correct for DE1-SoC/UARP systems.
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 */

#include "HPS_usleep.h"

#include "Util/watchdog.h"

#ifdef __ARRIA10__

//Base addresses of four possible timers
#define HPS_TIMER_SP0_BASE  0xFFC02700
#define HPS_TIMER_SP1_BASE  0xFFC02800
#define HPS_TIMER_OSC0_BASE 0xFFD00000
#define HPS_TIMER_OSC1_BASE 0xFFD00100

// Assume 125MHz timer if not defined
#define GUESS_TIMER_FREQ  125000000

#else

//Base addresses of four possible timers
#define HPS_TIMER_SP0_BASE  0xFFC08000
#define HPS_TIMER_SP1_BASE  0xFFC09000
#define HPS_TIMER_OSC0_BASE 0xFFD00000
#define HPS_TIMER_OSC1_BASE 0xFFD01000

// Assume 100MHz timer if not defined
#define GUESS_TIMER_FREQ 100000000

#endif

//Default to Timer SP1
#ifndef DEFAULT_USLEEP_TIMER_BASE
#define DEFAULT_USLEEP_TIMER_BASE HPS_TIMER_SP1_BASE
#endif

//Allow default timer frequency to be overridden.
#ifndef DEFAULT_USLEEP_TIMER_FREQ
#define DEFAULT_USLEEP_TIMER_FREQ GUESS_TIMER_FREQ
#endif

// Register Offsets
#define TIMER_LOAD   (0x00 / sizeof(unsigned int))
#define TIMER_CTRL   (0x08 / sizeof(unsigned int))
#define TIMER_RAWIRQ (0xA8 / sizeof(unsigned int))

#define TIMER_IRQENABLE (0 << 2)
#define TIMER_IRQMASKED (1 << 2)
#define TIMER_FREERUN   (0 << 1)
#define TIMER_ONESHOT   (1 << 1)
#define TIMER_DISABLED  (0 << 0)
#define TIMER_ENABLED   (1 << 0)


static volatile unsigned int* __timer = (unsigned int*)DEFAULT_USLEEP_TIMER_BASE;
static unsigned int __timerFreqMhz = (DEFAULT_USLEEP_TIMER_FREQ / 1E6);

//Change the timer used for usleep
// - Timer must be either of the SP0/SP1 timers, or either of the OSC0/OSC1 timers
// - Frequency is the clock rate at which time timer runs
// - If not called, defaults to SP1 timer running at 100MHz (Cyclone V) or 125MHz (Arria 10)
void selectTimer(HpsBridgeTimer timer, unsigned int freq) {
    switch (timer) {
    case HPS_TIMER_SP0 : __timer = (unsigned int*)HPS_TIMER_SP0_BASE;  break;
    case HPS_TIMER_SP1 : __timer = (unsigned int*)HPS_TIMER_SP1_BASE;  break;
    case HPS_TIMER_OSC0: __timer = (unsigned int*)HPS_TIMER_OSC0_BASE; break;
    case HPS_TIMER_OSC1: __timer = (unsigned int*)HPS_TIMER_OSC1_BASE; break;
    default:             __timer = (unsigned int*)DEFAULT_USLEEP_TIMER_BASE;
    }
    __timerFreqMhz = freq / 1E6;
}

//Microsecond sleep function based on Cyclone V HPS SP Timer 1
void usleep(int x) //Max delay ~2.09 seconds
{

    if (x <= 0) return; //For delays of 0 we just assume that we are done.
    if (x > 0x200000) x = 0x200000; //For delays longer than the max, set to max. 
    
    //Reset the watchdog before we sleep
    ResetWDT();
    
    //Convert x from microseconds to ticks
    __timer[TIMER_LOAD] = (((unsigned int)x) * __timerFreqMhz) - 1;
    //Re-initialise the timer
    __timer[TIMER_CTRL] = (TIMER_IRQMASKED | TIMER_ONESHOT | TIMER_DISABLED);
    __timer[TIMER_CTRL] = (TIMER_IRQMASKED | TIMER_ONESHOT | TIMER_ENABLED);
    //Wait until timer overflows
    while(!__timer[TIMER_RAWIRQ]);
    //Disable the timer
    __timer[TIMER_CTRL] = (TIMER_IRQMASKED | TIMER_ONESHOT | TIMER_DISABLED);

    //Reset the watchdog after we sleep
    ResetWDT();
}
