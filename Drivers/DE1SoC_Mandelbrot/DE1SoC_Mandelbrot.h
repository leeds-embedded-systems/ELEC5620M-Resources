/*
 * SoC Mandelbrot Controller Driver
 * ------------------------------------
 * Description: 
 * Driver for the Leeds SoC Computer Mandelbrot Controller
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 05/03/2018 | Creation of driver
 * 
 */

#ifndef DE1SOC_MANDELBROT_H_
#define DE1SOC_MANDELBROT_H_

//Include required header files
#include <stdint.h>
#include "Util/driver_ctx.h"
#include "DE1SoC_LT24/DE1SoC_LT24.h"

//Precision
typedef enum {
    MANDELBROT_FLOAT_PRECISION,
    MANDELBROT_DOUBLE_PRECISION
} MandelbrotPrecision;

// Driver context
typedef struct {
    // Context Header
    DrvCtx_t header;
    // Context Body
    volatile unsigned char* base;
    // Keeps track of previous configuration.
    MandelbrotPrecision precision;
    double magnitude;
    double radius;
    double xcentre;
    double ycentre;
} MandelbrotCtx_t, *PMandelbrotCtx_t;

//Function to initialise the Mandelbrot driver
// - Requires that the LT24 controller has already been initialised.
// - Returns 0 if successful
HpsErr_t Mandelbrot_initialise( void* base, PLT24Ctx_t lt24ctx, PMandelbrotCtx_t* pCtx );

//Check if driver initialised
// - returns true if initialised
bool Mandelbrot_isInitialised( PMandelbrotCtx_t ctx );

//Get Precision
// - returns MandelbrotPrecision value.
HpsErrExt_t Mandelbrot_getCalculationPrecision( PMandelbrotCtx_t ctx );

//Set Precision
// - Default to FLOAT
// - Sets the precision of the controller
// - returns 0 if successful
// - Once precision is changed, all coefficients (Zmax, Xmin, Ymin, Xstep, Ystep)
//   will automatically be reprogrammed.
HpsErr_t Mandelbrot_setCalculationPrecision( PMandelbrotCtx_t ctx, MandelbrotPrecision precision );

//Set Bounding Value
// - This typically never needs changing. It defaults to 2.
HpsErr_t Mandelbrot_setZnMax( PMandelbrotCtx_t ctx, double znMax );

//Set Coordinates
// - Sets coordinates and radius of pattern.
// - Change this to change the zoom.
HpsErr_t Mandelbrot_setCoordinates( PMandelbrotCtx_t ctx, double radius, double xcentre, double ycentre );

//Current iteration
// - Returns how many iterations have been made on the current pattern.
// - If >=0 is current iteration of this pattern
// - If <0 is error code
HpsErrExt_t Mandelbrot_currentIteration( PMandelbrotCtx_t ctx );

//Start new pattern
// - Resets the Mandelbrot generator and starts a new pattern.
// - Returns 0 if successfully started new pattern.
// - Ensure first that no iteration is currently running (Mandelbrot_iterationDone() returns SUCCESS).
HpsErr_t Mandelbrot_resetPattern( PMandelbrotCtx_t ctx );

//Start next iteration
// - Return 0 if successfully started iteration.
// - Call mandelbrot_interationDone() to check if finished.
HpsErr_t Mandelbrot_startIteration( PMandelbrotCtx_t ctx );

//Check if last iteration is done
// - Return 0 if successfully finished iteration.
// - Return ERR_BUSY if still running.
HpsErr_t Mandelbrot_iterationDone( PMandelbrotCtx_t ctx );

#endif /* DE1SOC_MANDELBROT_H_ */
