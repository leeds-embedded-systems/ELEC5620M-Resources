#include "DE1SoC_Servo/DE1SoC_Servo.h"
#include "DE1SoC_Addresses/DE1SoC_Addresses.h"

#include <stdlib.h>

// Fatal Status Check
// ==================
// This can be used to terminate the program (i.e. crash + reboot)
// in the event of an error
void exitOnFail(HpsErr_t status){
    if (ERR_IS_ERROR(status)) {
        exit((int)status); //Add breakpoint here to catch failure
    }
}

// Main Function
// =============
int main (void) {
    //Pointer variable to hold servo driver instance
    ServoCtx_t* servo0Ctx;
    //Initialise Servo Driver Structure
    exitOnFail(Servo_initialise(LSC_BASE_SERVO, SERVO_CHANNEL_0, &servo0Ctx));
    //Enable Servo 0.
    exitOnFail(Servo_enable(servo0Ctx, true));
    //Select double width pulse mode required for for SG-90 servo.
    exitOnFail(Servo_pulseWidthRange(servo0Ctx, true));
    //Default starting position
    signed char posn = 50;
    /* Main Run Loop */
    while (1) {        
        while (ERR_IS_BUSY(Servo_busy(servo0Ctx)));   //Wait while Servo 0 busy.
        exitOnFail(Servo_pulseWidth(servo0Ctx,posn));  //Update Servo 0 position
        //Other code... Maybe manipulate posn to control the servo position?
    }
}
