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

#ifndef DE1SoC_Servo_H_
#define DE1SoC_Servo_H_

//Include required header files
#include <stdint.h>
#include "Util/driver_ctx.h"
#include "Util/driver_gpio.h"

//Maximum number of servos
#define SERVO_MAX_COUNT  4

// Servo Driver Context
typedef struct {
    // Context Header
    DrvCtx_t header;
    // Context Body
    volatile unsigned int* base;
    GpioCtx_t gpio;
} ServoCtx_t;

//Function to initialise the Servo Controller
//  - Returns Util/error Code
//  - Returns context pointer to *ctx
HpsErr_t Servo_initialise( void* base, ServoCtx_t** pCtx );

//Check if driver initialised
// - returns true if initialised
bool Servo_isInitialised( ServoCtx_t* ctx );

//Enable/Disable a servo
// - "channel" is the number of the servo to control
// - "enable" is true to enable, false to disable.
//    - When a servo is disabled, its control pin can be used as an input for a switch or other device.
// - returns ERR_SUCCESS if successful
HpsErr_t Servo_enable( ServoCtx_t* ctx, unsigned int channel, bool enable );

//Read Back Servo Output Value
// - When the servo is disabled (see function above), the PWM pin become a general purpose input.
//   You can use this function to read the input value.
// - Reads will populate *value with the pin value, while mask can be used to select which pins to read.
// - When the servo is enabled, this function will return the current output value, and the pin
//   should not be used as an input.
HpsErr_t Servo_readInput( ServoCtx_t* ctx, unsigned int* value, unsigned int mask );

//Configure Pulse Width
// - Typically servos use a 1ms range, with 1ms representing +90deg and 2ms representing -90deg.
//   Some servos use a double width range or 2ms, +90deg being 0.5ms, and -90deg being 2.5ms
// - The servo controller supports both types. This function selects between them
// - Setting "doubleWidth" to false means 1ms range, setting "doubleWidth" to true means 2ms range.
// - Will return ERR_SUCCESS if successful.
HpsErr_t Servo_pulseWidthRange( ServoCtx_t* ctx, unsigned int channel, bool doubleWidth );

//Check if Busy
// - Servo position update is synchronous to PWM period.
//   If position changed while busy, last update will be lost.
// - Will return SERVO_BUSY if not ready for an update
// - Will return ERR_SUCCESS once ready.
HpsErr_t Servo_busy( ServoCtx_t* ctx, unsigned int channel );

//Update PWM Period
// - Period represents update rate of servo. This is typically
//   20ms or 40ms for cheap servos.
// - "period" is specified as a number in milliseconds from 1 to 255.
// - Will return ERR_SUCCESS if updated successfully.
HpsErr_t Servo_period( ServoCtx_t* ctx, unsigned int channel, unsigned char period );

//Update Centre Calibration
// - For a servo 1.5ms pulse width is nominally the centre for normal
//   servos, or the stop point for continuous rotation servos.
// - Component variation means 1.5ms may not be the exact centre point.
// - Calibration byte can move centre by +/-127 steps, each 1/256 ms.
//   e.g. 0 means 1.5ms centre, +26 means 1.6ms centre, etc.
// - "calibration" is the calibration byte for the servo.
// - Will return ERR_SUCCESS if updated successfully.
HpsErr_t Servo_calibrate( ServoCtx_t* ctx, unsigned int channel, signed char calibration );

//Set Pulse Width
// - Sets the width of the PWM pulse.
// - A value of 0 represents nominally 1.5ms
// - The width can be changed by +/-127 steps.
//   - If doubleWidth is enabled, each step is 1/128ms in size
//     e.g. 0 means 1.5ms, +26 means 1.7ms, etc.
//   - If doubleWidth is disabled, each step is 1/256ms in size
//     e.g. 0 means 1.5ms, +26 means 1.6ms, etc.
// - If the calibration byte is non-zero, the nominal pulse widths
//   will be added to the calibration offset internally.
// - "width" is the pulse width value for the servo.
// - Will return ERR_SUCCESS if updated successfully.
HpsErr_t Servo_pulseWidth( ServoCtx_t* ctx, unsigned int channel, signed char width );

#endif /*DE1SoC_Servo_H_*/
