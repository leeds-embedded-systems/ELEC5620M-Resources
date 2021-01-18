
#include "HPS_usleep.h"
#include "../HPS_Watchdog/HPS_Watchdog.h"

//Microsecond sleep function based on Cyclone V HPS SP Timer 1
void usleep(int x) //Max delay ~2.09 seconds
{
    volatile unsigned int *sptimer1_load = (unsigned int *)0xFFC09000;
    volatile unsigned int *sptimer1_ctrl = (unsigned int *)0xFFC09008;
    volatile unsigned int *sptimer1_irqs = (unsigned int *)0xFFC090A8; //Raw interrupt status (unmasked)
    
    if (x <= 0) return; //For delays of 0 we just assume that we are done.
    if (x > 0x200000) x = 0x200000; //For delays longer than the max, set to max. 
    
    //Reset the watchdog before we sleep
    ResetWDT();
    
    //Perform sleep operation
    *sptimer1_load = (x * 100) - 1; //SPT1 doesn't have prescaler. Clock rate is 100MHz, so times us by 100 to get ticks)
    *sptimer1_ctrl = 0x7;           //IRQ Masked, User Count Value, Enabled
    while(!(*sptimer1_irqs));       //Wait until timer overflows
    *sptimer1_ctrl = 0x4;           //IRQ Masked, Disable timer

    //Reset the watchdog after we sleep
    ResetWDT();
}
