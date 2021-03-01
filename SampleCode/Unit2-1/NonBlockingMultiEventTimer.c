    unsigned int lastTimerValue[2] = {0};
    const unsigned int requiredPeriod[2] = {100000000, 200000000}; 
    // Configuration 
    <Configure Timers, Peripherals, etc.>
    /* Main Run Loop */
    while(1) { 
        // Read the current time
        unsigned int currentTimerValue = *private_timer_value;
        // Check if it is time to blink
        if ((lastTimerValue[0] - currentTimerValue) >= requiredPeriod[0]) {
            // When the difference between the last time and current time is greater
            // than the required period. We use subtraction to prevent glitches
            // when the timer value overflows:
            //      e.g. (0x10 - 0xFFFFFFFF) & 0xFFFFFFFF = 0x11.
            // If the time elapsed is enough, perform our actions.
            <Perform some action here>
            // To avoid accumulation errors, we make sure to mark the last time
            // the task was run as when we expected to run it. Counter is going
            // down, so subtract the interval from the last time.
            lastTimerValue[0] = lastTimerValue[0] - requiredPeriod[0];
        }
        if ((lastTimerValue[1] - currentTimerValue) >= requiredPeriod[1]) {
            <Perform some other action here>
            lastTimerValue[1] = lastTimerValue[1] - requiredPeriod[1];
        }
        // --- You can have many other events here by giving each an if statement
        // --- and its own "last#TimerValue" and "#Period" variables, then using the
        // --- same structure as above.
        // Next, make sure we clear the private timer interrupt flag if it is set
        if (*private_timer_interrupt & 0x1) {
            // If the timer interrupt flag is set, clear the flag
            *private_timer_interrupt = 0x1;
        }
        // Finally, reset the watchdog timer. We can use the ResetWDT() macro.
        ResetWDT(); // This basically writes 0x76 to the watchdog reset register.
    }
}
