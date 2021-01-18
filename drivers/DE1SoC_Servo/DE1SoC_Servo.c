
#include "DE1SoC_Servo.h"

//
// Driver global static variables (visible only to this .c file)
//

//Driver Base Addresses
volatile unsigned int   *servo_base_ptr    = 0x0;  //0xFF2000C0
//Driver Initialised
bool servo_initialised = false;

//
// Useful Defines
//

//Control Bit Map
#define SERVO_ENABLE    (1 << 0)  //Output enab1e
#define SERVO_DOUBLEWID (1 << 1)  //Whether 1ms pulse or 2ms pulse
#define SERVO_READY     (1 << 2)  //Whether the servo is ready for changing
#define SERVO_INPUT     (1 << 4)  //Pin level of the servo io
#define SERVO_AVAILABLE (1 << 7)  //High if servo controller available

//LT24 Dedicated Address Offsets
#define SERVO_CONTROL  (0x00/sizeof(unsigned char))
#define SERVO_PERIOD   (0x01/sizeof(unsigned char))
#define SERVO_PULSEWID (0x02/sizeof(unsigned char))
#define SERVO_CENTRE   (0x03/sizeof(unsigned char))

//Validation function for servo ID
// - Returns true if an invalid servo ID
bool Servo_invalidID( unsigned int servo_id ) {
    volatile unsigned char* servo_ptr;
    if (servo_id >= SERVO_MAX_COUNT) return false;
    servo_ptr = (unsigned char*)&servo_base_ptr[servo_id]; //Get the csr for the requested servo
    return !(servo_ptr[SERVO_CONTROL] & SERVO_AVAILABLE);
}

//Function to initialise the Servo controller
signed int Servo_initialise( unsigned int base_address )
{    
    unsigned int id;
    //Set the local base address pointer
    servo_base_ptr = (unsigned int *) base_address;
    
    //Mark as initialised so later functions know we are ready
    servo_initialised = true;
    
    //Set all servos to disabled, single width, 20ms pulse width by default
    for (id = 0; id < SERVO_MAX_COUNT; id++) {
        if (Servo_invalidID(id)) continue; //Skip any invalid ones
        servo_base_ptr[id] = (20 << (8*SERVO_PERIOD)) | (0 << (8*SERVO_CONTROL));
    }
    
    //And done
    return SERVO_SUCCESS;
}

//Check if driver initialised
bool Servo_isInitialised() {
    return servo_initialised;
}

//Enable/Disable a servo
// - "servo_id" is the number of the servo to control
// - "set_enabled" is true to enable, false to disable.
// - returns SERVO_SUCCESS if successful
signed int Servo_enable( unsigned int servo_id, bool set_enabled) {
    volatile unsigned char* servo_ptr;
    //Sanity checks
    if (!Servo_isInitialised()) return SERVO_ERRORNOINIT;
    if (Servo_invalidID(servo_id)) return SERVO_INVALIDID;
    //Configure enable
    servo_ptr = (unsigned char*)&servo_base_ptr[servo_id]; //Get the csr for the requested servo
    if (set_enabled) {
        servo_ptr[SERVO_CONTROL] |=  SERVO_ENABLE;
    } else {
        servo_ptr[SERVO_CONTROL] &= ~SERVO_ENABLE;
    }
    return SERVO_SUCCESS;
}

//Read Back Servo Output Value
// - When the servo is disabled (see function above), the PWM pin become a general purpose input.
//   You can use this function to read the input value.
// - When the servo is enabled, this function will return the current output value, and the pin
//   should not be used as an input.
bool Servo_readInput( unsigned int servo_id ) {
    volatile unsigned char* servo_ptr;
    //Sanity checks
    if (!Servo_isInitialised()) return false;
    if (Servo_invalidID(servo_id)) return false;
    //Configure enable
    servo_ptr = (unsigned char*)&servo_base_ptr[servo_id]; //Get the csr for the requested servo
    return !!(servo_ptr[SERVO_CONTROL] & SERVO_INPUT);
}

//Configure Pulse Width
// - Typically servos use a 1ms range, with 1ms representing +90deg and 2ms representing -90deg.
//   Some servos use a double width range or 2ms, +90deg being 0.5ms, and -90deg being 2.5ms
// - The servo controller supports both types. This function selects between them
// - Setting "double_width" to false means 1ms range, setting "double_width" to true means 2ms range.
// - Will return SERVO_SUCCESS if successful.
signed int Servo_pulseWidthRange( unsigned int servo_id, bool double_width) {
    volatile unsigned char* servo_ptr;
    //Sanity checks
    if (!Servo_isInitialised()) return SERVO_ERRORNOINIT;
    if (Servo_invalidID(servo_id)) return SERVO_INVALIDID;
    //Configure enable
    servo_ptr = (unsigned char*)&servo_base_ptr[servo_id]; //Get the csr for the requested servo
    if (double_width) {
        servo_ptr[SERVO_CONTROL] |=  SERVO_DOUBLEWID;
    } else {
        servo_ptr[SERVO_CONTROL] &= ~SERVO_DOUBLEWID;
    }
    return SERVO_SUCCESS;
}

//Check if Busy
// - Servo position update is synchronous to PWM period.
//   If position changed while busy, last update will be lost.
// - Will return SERVO_BUSY if not ready for an update
// - Will return SERVO_SUCCESS once ready.
signed int Servo_busy( unsigned int servo_id ) {
    volatile unsigned char* servo_ptr;
    //Sanity checks
    if (!Servo_isInitialised()) return SERVO_ERRORNOINIT;
    if (Servo_invalidID(servo_id)) return SERVO_INVALIDID;
    //Check if busy
    servo_ptr = (unsigned char*)&servo_base_ptr[servo_id]; //Get the csr for the requested servo
    if (servo_ptr[SERVO_CONTROL] & SERVO_READY) { //Check if ready flag is set
        return SERVO_SUCCESS; //If so, not busy
    } else {
        return SERVO_BUSY; //Otherwise still busy
    }
}

//Update PWM Period
// - Period represents update rate of servo. This is typically
//   20ms or 40ms for cheap servos.
// - "period" is specified as a number in milliseconds from 1 to 255.
// - Will return SERVO_SUCCESS if updated successfully.
signed int Servo_period( unsigned int servo_id, unsigned char period) {
    //Sanity checks
    signed int status = Servo_busy(servo_id); //will also check if initialised or invalid servo id.
    if (status == SERVO_SUCCESS){
        //If ready, update the period
        volatile unsigned char* servo_ptr = (unsigned char*)&servo_base_ptr[servo_id]; //Get the csr for the requested servo
        servo_ptr[SERVO_PERIOD] = period;
    }
    return status;
}

//Update Centre Calibration
// - For a servo 1.5ms pulse width is nominally the centre for normal
//   servos, or the stop point for continuous rotation servos.
// - Component variation means 1.5ms may not be the exact centre point.
// - Calibration byte can move centre by +/-127 steps, each 1/256 ms.
//   e.g. 0 means 1.5ms centre, +26 means 1.6ms centre, etc.
// - "calibration" is the calibration byte for the servo.
// - Will return SERVO_SUCCESS if updated successfully.
signed int Servo_calibrate( unsigned int servo_id, signed char calibration) {
    //Sanity checks
    signed int status = Servo_busy(servo_id); //will also check if initialised or invalid servo id.
    if (status == SERVO_SUCCESS){
        //If ready, update the calibration
        volatile signed char* servo_ptr = (signed char*)&servo_base_ptr[servo_id]; //Get the csr for the requested servo
        servo_ptr[SERVO_CENTRE] = calibration;
    }
    return status;
}

//Set Pulse Width
// - Sets the width of the PWM pulse.
// - A value of 0 represents nominally 1.5ms
// - The width can be changed by +/-127 steps.
//   - If double_width is enabled, each step is 1/128ms in size
//     e.g. 0 means 1.5ms, +26 means 1.7ms, etc.
//   - If double_width is disabled, each step is 1/256ms in size
//     e.g. 0 means 1.5ms, +26 means 1.6ms, etc.
// - If the calibration byte is non-zero, the nominal pulse widths
//   will be added to the calibration offset internally.
// - "width" is the pulse width value for the servo.
// - Will return SERVO_SUCCESS if updated successfully.
signed int Servo_pulseWidth( unsigned int servo_id, signed char width) {
    //Sanity checks
    signed int status = Servo_busy(servo_id); //will also check if initialised or invalid servo id.
    if (status == SERVO_SUCCESS){
        //If ready, update the calibration
        volatile signed char* servo_ptr = (signed char*)&servo_base_ptr[servo_id]; //Get the csr for the requested servo
        servo_ptr[SERVO_PULSEWID] = width;
    }
    return status;
}

