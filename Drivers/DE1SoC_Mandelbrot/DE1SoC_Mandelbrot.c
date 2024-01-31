/*
 * DE1SoC_Mandelbrot.c
 *
 *  Created on: 5 Mar 2018
 *      Author: eentca
 */

#include "DE1SoC_Mandelbrot.h"

/*
 * Registers
 */

//Flags Bit Map
#define MANDELBROT_RESET   (1 << 2)
#define MANDELBROT_ITERATE (1 << 1)
#define MANDELBROT_INIT    (1 << 0)

//Control Bit Map
#define MANDELBROT_DBL_MODE   (1 << 0)

//Address Offsets
#define MANDELBROT_FLAGS     (0x00/sizeof(unsigned char))
#define MANDELBROT_CONTROL   (0x01/sizeof(unsigned char))
#define MANDELBROT_ITERATION (0x04/sizeof(unsigned char))
#define MANDELBROT_COEFFS    (0x08/sizeof(unsigned char))

//Coefficient
#define MANDELBROT_COEFF_ZNMAX(m) (0x00/sizeof(m))
#define MANDELBROT_COEFF_XMIN(m)  (0x08/sizeof(m))
#define MANDELBROT_COEFF_YMIN(m)  (0x10/sizeof(m))
#define MANDELBROT_COEFF_XSTEP(m) (0x18/sizeof(m))
#define MANDELBROT_COEFF_YSTEP(m) (0x20/sizeof(m))

//Helpers
#define MANDELBROT_YSIZE(RADIUS) (RADIUS * 2.0)                              //Narrowest dimension is twice radius
#define MANDELBROT_XSIZE(RADIUS) (((RADIUS * 2.0) * LT24_HEIGHT)/LT24_WIDTH) //Larger dimension is scaled proportionally

#define MANDELBROT_XMIN(XSIZE, XCENTRE)  (XCENTRE - (XSIZE/2))
#define MANDELBROT_YMIN(YSIZE, YCENTRE)  (YCENTRE - (YSIZE/2))

#define MANDELBROT_DEFAULT_MAG   2.0

/*
 * Internal Functions
 */

static void _Mandelbrot_setCoordinates( PMandelbrotCtx_t ctx, double radius, double xcentre, double ycentre ) {
    double xsize = MANDELBROT_XSIZE(radius);
    double ysize = MANDELBROT_YSIZE(radius);
    double xmin = MANDELBROT_XMIN(xsize, xcentre);
    double ymin = MANDELBROT_YMIN(ysize, ycentre);
    double xstep = xsize / LT24_HEIGHT;
    double ystep = ysize / LT24_WIDTH;
    //Update internally
    ctx->magnitude = radius;
    ctx->xcentre = xcentre;
    ctx->ycentre = ycentre;
    //Update device
    if (ctx->precision == MANDELBROT_FLOAT_PRECISION) {
        //If single precision, get pointer as float
        volatile float* coeffsPtr = (float*)&ctx->base[MANDELBROT_COEFFS];
        //And store reduced precision values
        coeffsPtr[MANDELBROT_COEFF_XMIN(float)] = (float)xmin;
        coeffsPtr[MANDELBROT_COEFF_YMIN(float)] = (float)ymin;
        coeffsPtr[MANDELBROT_COEFF_XSTEP(float)] = (float)xstep;
        coeffsPtr[MANDELBROT_COEFF_YSTEP(float)] = (float)ystep;
    } else {
        //If double precision, get pointer as double
    	volatile double* coeffsPtr = (double*)&ctx->base[MANDELBROT_COEFFS];
        //And store double values
        coeffsPtr[MANDELBROT_COEFF_XMIN(double)] = xmin;
        coeffsPtr[MANDELBROT_COEFF_YMIN(double)] = ymin;
        coeffsPtr[MANDELBROT_COEFF_XSTEP(double)] = xstep;
        coeffsPtr[MANDELBROT_COEFF_YSTEP(double)] = ystep;
    }
}

static void _Mandelbrot_setZnMax( PMandelbrotCtx_t ctx, double znMax ) {
    //Update internally
    ctx->magnitude = znMax;
    //Update device
    if (ctx->precision == MANDELBROT_FLOAT_PRECISION) {
        //If single precision, get pointer as float
    	volatile float* coeffsPtr = (float*)&ctx->base[MANDELBROT_COEFFS];
        //And store reduced precision value (square it as register expects zn^2)
        coeffsPtr[MANDELBROT_COEFF_ZNMAX(float)] = (float)(znMax * znMax);
    } else {
        //If double precision, get pointer as double
    	volatile double* coeffsPtr = (double*)&ctx->base[MANDELBROT_COEFFS];
        //And store double value (square it as register expects zn^2)
        coeffsPtr[MANDELBROT_COEFF_ZNMAX(double)] = (znMax * znMax);
    }
}

static void _Mandelbrot_setCalculationPrecision( PMandelbrotCtx_t ctx, MandelbrotPrecision precision ) {
    ctx->precision = precision;
    if (precision == MANDELBROT_FLOAT_PRECISION) {
        ctx->base[MANDELBROT_CONTROL] = 0;
    } else {
        ctx->base[MANDELBROT_CONTROL] = MANDELBROT_DBL_MODE;
    }
    //Reload existing coordinates to ensure correct precision.
    _Mandelbrot_setZnMax(ctx, ctx->magnitude);
    _Mandelbrot_setCoordinates(ctx, ctx->radius, ctx->xcentre, ctx->ycentre);
}

/*
 * User Facing APIs
 */

//Function to initialise the Mandelbrot driver
// - Requires that the LT24 controller has already been initialised.
// - Returns 0 if successful
HpsErr_t Mandelbrot_initialise( unsigned int base, PLT24Ctx_t lt24ctx, PMandelbrotCtx_t* pCtx ) {
    //Ensure user pointers valid.
    if (!base) return ERR_NULLPTR;
    if (!pointerIsAligned(base, sizeof(unsigned int))) return ERR_ALIGNMENT;
    //Check if the LT24 display has been initialised (required)
    if (!LT24_isInitialised(lt24ctx)) return ERR_NOINIT;
    //Allocate the driver context, validating return value.
    HpsErr_t status = DriverContextAllocate(pCtx);
    if (IS_ERROR(status)) return status;
    //Save base address pointers
    PMandelbrotCtx_t ctx = *pCtx;
    ctx->base = (unsigned int*)base;
    //Set default co-ordinates
    ctx->magnitude  =  2.00;
    ctx->radius     =  2.60;
    ctx->xcentre    = -0.75;
    ctx->ycentre    =  0.00;
    //Start as float precision (will also write our initial co-ordinates)
    _Mandelbrot_setCalculationPrecision(ctx, MANDELBROT_FLOAT_PRECISION);
    //And done
    return ERR_SUCCESS;
}

//Check if driver initialised
// - returns true if initialised
bool Mandelbrot_isInitialised(PMandelbrotCtx_t ctx) {
    return DriverContextCheckInit(ctx);
}

//Get Precision
// - returns true if double precision
HpsErrExt_t Mandelbrot_getCalculationPrecision( PMandelbrotCtx_t ctx ) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (IS_ERROR(status)) return status;
    //Return precision
    return ctx->precision;
}

//Set Precision
// - Sets the precision of the controller
// - returns 0 if successful
// - Once precision is changed, *all* coefficients (Zmax, Xmin, Ymin, Xstep, Ystep)
//   will automatically be updated.
HpsErr_t Mandelbrot_setCalculationPrecision( PMandelbrotCtx_t ctx, PMandelbrotCtx_t precision ) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (IS_ERROR(status)) return status;
    //Set precision
    _Mandelbrot_setCalculationPrecision(ctx, precision);
    return ERR_SUCCESS;
}

//Set Bounding Value
// - This typically never needs changing. It defaults to 2.
HpsErr_t Mandelbrot_setZnMax( PMandelbrotCtx_t ctx, double znMax ) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (IS_ERROR(status)) return status;
    //Set magnitude
    _Mandelbrot_setZnMax(ctx, znMax);
    return ERR_SUCCESS;
}

//Set Coordinates
// - Sets coordinates and radius of pattern.
// - Change this to change the zoom.
HpsErr_t Mandelbrot_setCoordinates( PMandelbrotCtx_t ctx, double radius, double xcentre, double ycentre ) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (IS_ERROR(status)) return status;
    //Set coordinates
    _Mandelbrot_setCoordinates(ctx, radius, xcentre, ycentre);
    return ERR_SUCCESS;
}

//Current iteration
// - Returns how many iterations have been made on the current pattern.
// - If >=0 is current iteration of this pattern
// - If <0 is error code
HpsErrExt_t Mandelbrot_currentIteration( PMandelbrotCtx_t ctx ) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (IS_ERROR(status)) return status;
    //Return current iteration
    unsigned int iteration = *((unsigned int*)&ctx->base[MANDELBROT_ITERATION]);
    return (iteration & INT32_MIN); //Ensure the iteration value doesn't accidentally become error status code.
}

//Start new pattern
// - Resets the Mandelbrot generator and starts a new pattern.
// - Returns 0 if successfully started new pattern.
// - Ensure first that no iteration is currently running (Mandelbrot_iterationDone() returns SUCCESS).
HpsErr_t Mandelbrot_resetPattern( PMandelbrotCtx_t ctx ) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (IS_ERROR(status)) return status;
    //Check if generator is busy with existing iteration
    if (!(ctx->base[MANDELBROT_FLAGS] & MANDELBROT_ITERATE)) {
        //If busy, don't allow reset:
        return ERR_BUSY;
        /* -- Reset not enabled, Mandelbrot IP core needs upgrading --
        //If busy, force reset
        ctx->base[MANDELBROT_FLAGS] = MANDELBROT_RESET; //Reset generator
        while (!(ctx->base[MANDELBROT_FLAGS] & MANDELBROT_ITERATE)); //Wait until reset complete.
        */
    }
    //Initialise the generator
    ctx->base[MANDELBROT_FLAGS] = MANDELBROT_INIT; //Reset generator
    //Wait for it to be ready
    while (!(ctx->base[MANDELBROT_FLAGS] & MANDELBROT_INIT)); //Wait until initialisation complete.
    //And done.
    return ERR_SUCCESS;
}

//Start next iteration
// - Return 0 if successfully started iteration.
// - Call mandelbrot_interationDone() to check if finished.
HpsErr_t Mandelbrot_startIteration( PMandelbrotCtx_t ctx ) {
    //Check if busy (will also validate context)
    HpsErr_t status = Mandelbrot_iterationDone(ctx);
    if (IS_ERROR(status)) return status;
    //Perform iteration.
    ctx->base[MANDELBROT_FLAGS] = MANDELBROT_ITERATE;
    return ERR_SUCCESS;
}

//Check if last iteration is done
// - Return 0 if successfully finished iteration.
// - Return ERR_BUSY if still running.
HpsErr_t Mandelbrot_iterationDone( PMandelbrotCtx_t ctx ) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (IS_ERROR(status)) return status;
    //Check if generator is initialised
    unsigned char flags = ctx->base[MANDELBROT_FLAGS];
    if (!(flags & MANDELBROT_INIT)) return ERR_NOTREADY;
    //Check if generator is busy with existing iteration
    if (!(flags & MANDELBROT_ITERATE)) return ERR_BUSY;
    //Not busy if reached this far.
    return ERR_SUCCESS;
}

