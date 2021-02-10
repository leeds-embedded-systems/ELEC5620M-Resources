/*
 * HPS SP1 Timer Based "usleep"
 * ------------------------------
 * Description: 
 * The POSIX usleep() function does not exist for bare
 * metal applications in DS-5. This library adds a similar
 * function which allows the processor to be stalled for
 * x microseconds.
 * 
 * Delays up to ~2.09 seconds are supported. This is to
 * ensure the watchdog won't time out before the delay is
 * finished. This corresponds to a value of 0x200000 us.
 * The watchdog will be reset just before the delay starts
 * and just after it finishes.
 * 
 * The delay is performed using the HPS SP1 Timer at base
 * address 0xFFC09000. This is a hardware timer in the SoC
 * HPS bridge, so does not conflict with the ARM A9 private
 * timer module.
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 */

#ifndef HPS_USLEEP_H_
#define HPS_USLEEP_H_

//Delay for x microseconds
void usleep(int x);

#endif //DELAY_H_
