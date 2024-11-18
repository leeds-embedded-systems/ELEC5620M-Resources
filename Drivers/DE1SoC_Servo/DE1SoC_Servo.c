/* 
 * Servo Controller Driver
 * ------------------------------
 * Description: 
 * Driver for the Servo Controller in the DE1-SoC
 * Computer
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 31/01/2024 | Update to new driver contexts
 * 05/12/2018 | Add Servo_readInput Function to get value
 *            | of servo GPIO pin when used as input
 * 31/10/2017 | Creation of driver
 *
 */

#include "DE1SoC_Servo.h"
#include "Util/bit_helpers.h"

//
// Register Maps
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

/*
 * Internal Functions
 */

//Validation function for servo ID
// - Returns true if an invalid servo ID
static bool _Servo_invalidID( ServoCtx_t* ctx, unsigned int channel ) {
    if (channel >= SERVO_MAX_COUNT) return false;
    volatile unsigned char* servo_ptr = (unsigned char*)&ctx->base[channel]; //Get the csr for the requested servo
    return !(servo_ptr[SERVO_CONTROL] & SERVO_AVAILABLE);
}

static void _Servo_enable( ServoCtx_t* ctx, unsigned int channel, bool enable) {
    //Get the csr for the requested servo
    volatile unsigned char* servo_ptr = (unsigned char*)&ctx->base[channel]; 
    if (enable) {
        servo_ptr[SERVO_CONTROL] |=  SERVO_ENABLE;
    } else {
        servo_ptr[SERVO_CONTROL] &= ~SERVO_ENABLE;
    }
}

// Cleanup function called when driver destroyed.
static void _Servo_cleanup( ServoCtx_t* ctx ) {
    if (ctx->base) {
        // Disable all servos
        for (unsigned int channel = 0; channel < SERVO_MAX_COUNT; channel++) {
            if (_Servo_invalidID(ctx, channel)) continue; //Skip any invalid ones
            _Servo_enable(ctx, channel, false);
        }
    }
}

/*
 * User Facing APIs
 */

//Function to initialise the Servo controller
HpsErr_t Servo_initialise( void* base, ServoCtx_t** pCtx ) {
    //Ensure user pointers valid. dataBase can be NULL.
    if (!base) return ERR_NULLPTR;
    if (!pointerIsAligned(base, sizeof(unsigned int))) return ERR_ALIGNMENT;
    //Allocate the driver context, validating return value.
    HpsErr_t status = DriverContextAllocateWithCleanup(pCtx, &_Servo_cleanup);
    if (ERR_IS_ERROR(status)) return status;
    //Save base address pointers
    ServoCtx_t* ctx = *pCtx;
    ctx->base = (unsigned int*)base;
    //Populate the GPIO structure. We only have the getInput function.
    ctx->gpio.ctx = ctx;
    ctx->gpio.getInput = (GpioReadFunc_t)&Servo_readInput;
    //Set all servos to disabled, single width, 20ms pulse width by default
    for (unsigned int channel = 0; channel < SERVO_MAX_COUNT; channel++) {
        if (_Servo_invalidID(ctx, channel)) continue; //Skip any invalid ones
        ctx->base[channel] = (20 << (8*SERVO_PERIOD)) | (0 << (8*SERVO_CONTROL));
    }
    //Mark as initialised so later functions know we are ready
    DriverContextSetInit(ctx);
    return ERR_SUCCESS;
}

//Check if driver initialised
bool Servo_isInitialised( ServoCtx_t* ctx ) {
    return DriverContextCheckInit(ctx);
}

//Enable/Disable a servo
// - "channel" is the number of the servo to control
// - "set_enabled" is true to enable, false to disable.
// - returns ERR_SUCCESS if successful
HpsErr_t Servo_enable( ServoCtx_t* ctx, unsigned int channel, bool enable) {
    //Ensure context valid and initialised and channel is valid
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    if (_Servo_invalidID(ctx, channel)) return ERR_NOTFOUND;
    //Configure enable
    _Servo_enable(ctx, channel, enable);
    return ERR_SUCCESS;
}

//Read Back Servo Output Value
// - When the servo is disabled (see function above), the PWM pin become a general purpose input.
//   You can use this function to read the input value.
// - Reads will populate *value with the pin value, while mask can be used to select which pins to read.
// - When the servo is enabled, this function will return the current output value, and the pin
//   should not be used as an input.
HpsErr_t Servo_readInput( ServoCtx_t* ctx, unsigned int* value, unsigned int mask ) {
    // Validate inputs
    if (!value) return ERR_NULLPTR;
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    // Read servo pins
    unsigned int _value = 0;
    for (unsigned int channel = 0; channel < SERVO_MAX_COUNT; channel = channel + 1) {
        // Skip invalid or unwanted channels
        if (_Servo_invalidID(ctx, channel)) continue;
        if (!(mask & (1 << channel))) continue;
        //Get the csr for the requested servo
        volatile unsigned char* servo_ptr = (unsigned char*)&ctx->base[channel];
        if (servo_ptr[SERVO_CONTROL] & SERVO_INPUT) {
            //If the input is high, or it in to our return value.
            _value |= (1 << channel);
        }
    }
    // Return the value.
    *value = _value;
    return ERR_SUCCESS;
}

//Configure Pulse Width
// - Typically servos use a 1ms range, with 1ms representing +90deg and 2ms representing -90deg.
//   Some servos use a double width range or 2ms, +90deg being 0.5ms, and -90deg being 2.5ms
// - The servo controller supports both types. This function selects between them
// - Setting "double_width" to false means 1ms range, setting "double_width" to true means 2ms range.
// - Will return ERR_SUCCESS if successful.
HpsErr_t Servo_pulseWidthRange( ServoCtx_t* ctx, unsigned int channel, bool double_width ) {
    //Ensure context valid and initialised and channel is valid
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    if (_Servo_invalidID(ctx, channel)) return ERR_NOTFOUND;
    //Configure enable
    volatile unsigned char* servo_ptr = (unsigned char*)&ctx->base[channel]; //Get the csr for the requested servo
    if (double_width) {
        servo_ptr[SERVO_CONTROL] |=  SERVO_DOUBLEWID;
    } else {
        servo_ptr[SERVO_CONTROL] &= ~SERVO_DOUBLEWID;
    }
    return ERR_SUCCESS;
}

//Check if Busy
// - Servo position update is synchronous to PWM period.
//   If position changed while busy, last update will be lost.
// - Will return ERR_BUSY if not ready for an update
// - Will return ERR_SUCCESS once ready.
HpsErr_t Servo_busy( ServoCtx_t* ctx, unsigned int channel ) {
    //Ensure context valid and initialised and channel is valid
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    if (_Servo_invalidID(ctx, channel)) return ERR_NOTFOUND;
    //Check if busy
    volatile unsigned char* servo_ptr = (unsigned char*)&ctx->base[channel]; //Get the csr for the requested servo
    return (servo_ptr[SERVO_CONTROL] & SERVO_READY) ? ERR_SUCCESS : ERR_BUSY; //Otherwise still busy
}

//Update PWM Period
// - Period represents update rate of servo. This is typically
//   20ms or 40ms for cheap servos.
// - "period" is specified as a number in milliseconds from 1 to 255.
// - Will return ERR_SUCCESS if updated successfully.
HpsErr_t Servo_period( ServoCtx_t* ctx, unsigned int channel, unsigned char period) {
    //Ensure controller not busy as well as context and channel validity.
    HpsErr_t status = Servo_busy(ctx, channel);
    if (ERR_IS_ERROR(status)) return status;
    //If ready, update the period
    volatile unsigned char* servo_ptr = (unsigned char*)&ctx->base[channel]; //Get the csr for the requested servo
    servo_ptr[SERVO_PERIOD] = period;
    return ERR_SUCCESS;
}

//Update Centre Calibration
// - For a servo 1.5ms pulse width is nominally the centre for normal
//   servos, or the stop point for continuous rotation servos.
// - Component variation means 1.5ms may not be the exact centre point.
// - Calibration byte can move centre by +/-127 steps, each 1/256 ms.
//   e.g. 0 means 1.5ms centre, +26 means 1.6ms centre, etc.
// - "calibration" is the calibration byte for the servo.
// - Will return ERR_SUCCESS if updated successfully.
HpsErr_t Servo_calibrate( ServoCtx_t* ctx, unsigned int channel, signed char calibration) {
    //Ensure controller not busy as well as context and channel validity.
    HpsErr_t status = Servo_busy(ctx, channel);
    if (ERR_IS_ERROR(status)) return status;
    //If ready, update the calibration
    volatile signed char* servo_ptr = (signed char*)&ctx->base[channel]; //Get the csr for the requested servo
    servo_ptr[SERVO_CENTRE] = calibration;
    return ERR_SUCCESS;
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
// - Will return ERR_SUCCESS if updated successfully.
HpsErr_t Servo_pulseWidth( ServoCtx_t* ctx, unsigned int channel, signed char width) {
    //Ensure controller not busy as well as context and channel validity.
    HpsErr_t status = Servo_busy(ctx, channel);
    if (ERR_IS_ERROR(status)) return status;
    //If ready, update the calibration
    volatile signed char* servo_ptr = (signed char*)&ctx->base[channel]; //Get the csr for the requested servo
    servo_ptr[SERVO_PULSEWID] = width;
    return ERR_SUCCESS;
}

