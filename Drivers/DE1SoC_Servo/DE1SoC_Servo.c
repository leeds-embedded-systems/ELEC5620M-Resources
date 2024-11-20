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
 * 20/11/2024 | Separate Servo Channel Instances
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
#define SERVO_POSITN   (0x02/sizeof(unsigned char))
#define SERVO_CENTRE   (0x03/sizeof(unsigned char))

#define SERVO_CHANNEL_OFFSET(ch) (ch * sizeof(unsigned int))

/*
 * Internal Functions
 */

// Cleanup function called when driver destroyed.
static void _Servo_cleanup( ServoCtx_t* ctx ) {
    if (ctx->base) {
        // Disable servo
        ctx->base[SERVO_CONTROL] = 0;
    }
}

/*
 * User Facing APIs
 */

//Function to initialise the Servo controller
HpsErr_t Servo_initialise( void* base, ServoChannel channel, ServoCtx_t** pCtx ) {
    //Ensure user pointers valid. dataBase can be NULL.
    if (!base) return ERR_NULLPTR;
    if (!pointerIsAligned(base, sizeof(unsigned int))) return ERR_ALIGNMENT;
    if ((channel >= SERVO_MAX_COUNT) || (channel < 0)) return ERR_BADID;
    //Allocate the driver context, validating return value.
    HpsErr_t status = DriverContextAllocateWithCleanup(pCtx, &_Servo_cleanup);
    if (ERR_IS_ERROR(status)) return status;
    //Save base address pointers
    ServoCtx_t* ctx = *pCtx;
    ctx->base = (unsigned char*)base + SERVO_CHANNEL_OFFSET(channel);
    ctx->channel = channel;
    //Check if servo is available for use
    if (!(ctx->base[SERVO_CONTROL] & SERVO_AVAILABLE)) return DriverContextInitFail(pCtx, ERR_NOTFOUND);
    //Populate the GPIO structure. We only have the getInput function.
    ctx->gpio.ctx = ctx;
    ctx->gpio.getInput = (GpioReadFunc_t)&Servo_readInput;
    //Set servo to disabled, single width, 20ms pulse width by default
    ctx->base[SERVO_CONTROL] = 0;
    ctx->base[SERVO_PERIOD ] = 20;
    ctx->base[SERVO_POSITN ] = 0;
    ctx->base[SERVO_CENTRE ] = 0;
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
HpsErr_t Servo_enable( ServoCtx_t* ctx, bool enable) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Configure enable
    ctx->base[SERVO_CONTROL] = MaskModify(ctx->base[SERVO_CONTROL], enable, 0x1U, SERVO_ENABLE);
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
    // Read servo pin input flag
    *value = MaskExtract(ctx->base[SERVO_CONTROL], mask & 0x1U, SERVO_INPUT);
    return ERR_SUCCESS;
}

//Configure Pulse Width
// - Typically servos use a 1ms range, with 1ms representing +90deg and 2ms representing -90deg.
//   Some servos use a double width range or 2ms, +90deg being 0.5ms, and -90deg being 2.5ms
// - The servo controller supports both types. This function selects between them
// - Setting "double_width" to false means 1ms range, setting "double_width" to true means 2ms range.
// - Will return ERR_SUCCESS if successful.
HpsErr_t Servo_pulseWidthRange( ServoCtx_t* ctx, bool double_width ) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Configure double-width flag
    ctx->base[SERVO_CONTROL] = MaskModify(ctx->base[SERVO_CONTROL], double_width, 0x1U, SERVO_DOUBLEWID);
    return ERR_SUCCESS;
}

//Check if Busy
// - Servo position update is synchronous to PWM period.
//   If position changed while busy, last update will be lost.
// - Will return ERR_BUSY if not ready for an update
// - Will return ERR_SUCCESS once ready.
HpsErr_t Servo_busy( ServoCtx_t* ctx ) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Check if ready flag is set
    return MaskCheck(ctx->base[SERVO_CONTROL], 0x1U, SERVO_READY) ? ERR_SUCCESS : ERR_BUSY;
}

//Update PWM Period
// - Period represents update rate of servo. This is typically
//   20ms or 40ms for cheap servos.
// - "period" is specified as a number in milliseconds from 1 to 255.
// - Will return ERR_SUCCESS if updated successfully.
HpsErr_t Servo_period( ServoCtx_t* ctx, unsigned char period) {
    //Ensure controller not busy as well as context validity.
    HpsErr_t status = Servo_busy(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //If ready, update the period
    ctx->base[SERVO_PERIOD] = period;
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
HpsErr_t Servo_calibrate( ServoCtx_t* ctx, signed char calibration) {
    //Ensure controller not busy as well as context validity.
    HpsErr_t status = Servo_busy(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //If ready, update the calibration
    ctx->base[SERVO_CENTRE] = calibration;
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
HpsErr_t Servo_pulseWidth( ServoCtx_t* ctx, signed char width) {
    //Ensure controller not busy as well as context validity.
    HpsErr_t status = Servo_busy(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //If ready, update the calibration
    ctx->base[SERVO_POSITN] = width;
    return ERR_SUCCESS;
}

