#include "DE1SoC_Servo/DE1SoC_Servo.h"
#include "DE1SoC_Addresses/DE1SoC_Addresses.h"

#include <stdlib.h>

// Fatal Status Check
// ==================
// This can be used to terminate the program (i.e. crash + reboot)
// if the "status" value returned from a driver doesn't match the
// "successStatus" value.

void exitOnFail(signed int status, signed int successStatus){
    if (status != successStatus) {
        exit((int)status); //Add breakpoint here to catch failure
    }
}

// Main Function
// =============

int main (void) {
    //Initialise Servo Driver
    PServoCtx_t servoCtx;
    exitOnFail(    //Initialise Servo Controller
            Servo_initialise(LSC_BASE_SERVO, &servoCtx),   
            ERR_SUCCESS);                                  //Exit if not successful
    exitOnFail(
            Servo_enable(servoCtx, 0, true),               //Enable Servo 0.
            ERR_SUCCESS);                                  //Exit if not successful
    exitOnFail(
            Servo_pulseWidthRange(servoCtx, 0, true),      //Double pulse width.
            ERR_SUCCESS);                                  //Exit if not successful

    signed char posn = 50;
    /* Main Run Loop */
    while (1) {        
        while (IS_BUSY(Servo_busy(servoCtx, 0)));       //Wait while Servo 0 busy.
        exitOnFail(
                Servo_pulseWidth(servoCtx,0,posn),      //Update Servo 0 position
                ERR_SUCCESS);                           //Exit if not successful
        //Other code... Maybe manipulate posn to control the servo position?
    }
}
