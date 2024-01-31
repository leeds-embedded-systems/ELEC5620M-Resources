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
 * function and providing a base address.
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

#ifndef HPS_USLEEP_H_
#define HPS_USLEEP_H_

//Delay for x microseconds
void usleep(int x);


//Change the timer used for usleep
// - Timer must be either of the SP0/SP1 timers, or either of the OSC0/OSC1 timers
// - Frequency is the clock rate at which time timer runs
// - If not called, defaults to SP1 timer running at 100MHz (Cyclone V) or 125MHz (Arria 10)
typedef enum {
    HPS_TIMER_SP0,
    HPS_TIMER_SP1,
    HPS_TIMER_OSC0,
    HPS_TIMER_OSC1
} HpsBridgeTimer;

void selectTimer(HpsBridgeTimer timer, unsigned int freq);

#endif //DELAY_H_
