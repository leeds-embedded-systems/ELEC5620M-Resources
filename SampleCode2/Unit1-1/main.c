/*
 * main.c
 *
 * This program is a simple piece of code to test the Watchdog Timer.
 *
 * The watchdog will be reset any time a button is pressed.
 *
 * The value of the watchdog timer is displayed on the red LEDs.
 *
 *  Created on: 13 Jan 2018
 *      Author: Doug from Up.
 *       Notes: Squirrel!
 */


int main(void) {
    /*
     *  Declare pointers to I/O registers (volatile keyword means memory not cached)
     */

    // WDT Current Value  Register (l4wd0: wdt_ccvr)
    volatile unsigned int *wdt_ccvr = (unsigned int *) 0xFFD02008;
    // WDT Counter Reload Register (l4wd0: wdt_crr)
    volatile unsigned int *wdt_crr  = (unsigned int *) 0xFFD0200C;
    // Red LEDs base address
    volatile int *LEDR_ptr = (int *) 0xFF200000;
    // KEY buttons base address
    volatile int *KEY_ptr  = (int *) 0xFF200050;

    /*
     *  Primary Run Loop.
     *  Bare-Metal Applications use an infinite loop to keep executing (no OS).
     */
     while(1) {
        // Read WDT counter value, and display it on the red LEDs (scaled to fit)
        *LEDR_ptr = *wdt_ccvr >> 16;
        // Check if any of the buttons have been pressed.
        // Each of the lower 4 bits represents one of the keys.
        // A key is pressed if the corresponding bit is 1.
        if (*KEY_ptr & 0xF) {
            // If any key pressed
            *wdt_crr = 0x76; // Reset the Watchdog Timer
        }
    }
}

