/* 1-2-Summation 
 * -----------------
 * This program performs the following on the ARM A9 and DE1-SoC Computer:
 * 	1. Initialises pointers to peripheral addresses
 * 	2. Loops continuously, sets LEDs to the summation of KEY3-0 input.
 * 	3. Resets the watchdog timer value at the end of the main loop.
 */
#include "HPS_Watchdog/HPS_Watchdog.h"

int main(void) {
    //Peripheral Base Address Pointers
    //Red LEDs base
    volatile unsigned int *LEDR_ptr = (unsigned int *) 0xFF200000;
    // KEY buttons base
    volatile unsigned int *KEY_ptr  = (unsigned int *) 0xFF200050;
    //Local Variables
    unsigned int N;
    unsigned int sum;
    //Run Loop
    while (1) {
        // Set N to KEY[3:0] input value.
        N = *KEY_ptr & 0x0F;
        // Reset the sum value
        sum = 0;             
        // Summation loop.
        while ( N != 0 ) {   
            sum = sum + N;
            N--;
        }
        // Set LED output value to sum result.
        *LEDR_ptr = sum;     
        // Reset the watchdog timer
        HPS_ResetWatchdog();     
    }
}


// Recursively sum numbers 0 to N
unsigned int recursiveSum(unsigned int N) {
    if (N == 0) { //If N is 0, we have reached deepest level
        return 0; //The sum of 0 + 0 is 0.
    } else { //Otherwise we need to perform another level
        return N + recursiveSum(N-1); //Return N + sum(0 to N-1)
    }
}
