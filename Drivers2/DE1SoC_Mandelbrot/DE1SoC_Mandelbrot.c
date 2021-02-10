/*
 * DE1SoC_Mandelbrot.c
 *
 *  Created on: 5 Mar 2018
 *      Author: eentca
 */

#include "DE1SoC_Mandelbrot.h"
#include "../DE1SoC_LT24/DE1SoC_LT24.h"

//
// Driver global static variables (visible only to this .c file)
//

//Driver Base Addresses
volatile unsigned char *mandelbrot_base_ptr = 0x0;  //0xFF200090

//Driver Initialised
bool mandelbrot_initialised = false;

//
// Useful Defines
//

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

// Internal globals
bool   mandelbrot_doublePrecision;
double mandelbrot_radius;
double mandelbrot_ycentre;
double mandelbrot_xcentre;
double mandelbrot_magnitude;

//Function to initialize the Mandelbrot driver
// - Requires that LT24_initialise has been called already.
// - Returns 0 if successful
signed int Mandelbrot_initialise( unsigned int mandelbrot_base_address ) {
    //Store the base address pointer.
    mandelbrot_base_ptr = (unsigned char *)mandelbrot_base_address;
    
    //Check if the LT24 display has been initialised (required)
    if (!LT24_isInitialised()) {
        return MANDELBROT_ERRORNOINIT;
    }
    
    //Mark as initialised so later functions know we are ready
    mandelbrot_initialised = true;
    
    //Default co-ordinates
    mandelbrot_radius    =  2.60;
    mandelbrot_xcentre   = -0.75;
    mandelbrot_ycentre   =  0.00;
    
    //Default Bounding
    mandelbrot_magnitude =  2.00;
    
    //Start as float precision (will auto-set co-ordinates)
    Mandelbrot_setCalculationPrecision(MANDELBROT_FLOAT_PRECISION);
        
    //And done
    return MANDELBROT_SUCCESS;
}

//Check if driver initialised
// - returns true if initialised
bool Mandelbrot_isInitialised() {
    return mandelbrot_initialised;
}

//Get Precision
// - returns true if double precision
bool Mandelbrot_getCalculationPrecision( ) {
    return mandelbrot_doublePrecision;
}

//Set Precision
// - Sets the precision of the controller
// - returns 0 if successful
// - Once precision is changed, *all* coefficients (Zmax, Xmin, Ymin, Xstep, Ystep)
//   will automatically be updated.
signed int Mandelbrot_setCalculationPrecision( bool doublePrecision ) {
    if (!Mandelbrot_isInitialised()) return MANDELBROT_ERRORNOINIT;
    mandelbrot_doublePrecision = doublePrecision;
    if (doublePrecision == MANDELBROT_FLOAT_PRECISION) {
        mandelbrot_base_ptr[MANDELBROT_CONTROL] = 0;
    } else {
        mandelbrot_base_ptr[MANDELBROT_CONTROL] = MANDELBROT_DBL_MODE;
    }
    Mandelbrot_setZnMax(mandelbrot_magnitude);
    Mandelbrot_setCoordinates(mandelbrot_radius, mandelbrot_xcentre, mandelbrot_ycentre);
    return MANDELBROT_SUCCESS;
}

//Set Bounding Value
// - This typically never needs changing. It defaults to 2.
signed int Mandelbrot_setZnMax( double znMax ) {
    //Check if initialised
    if (!Mandelbrot_isInitialised()) return MANDELBROT_ERRORNOINIT;
    //Update internally
    mandelbrot_magnitude = znMax;
    //Update device
    if (Mandelbrot_getCalculationPrecision() == MANDELBROT_FLOAT_PRECISION) {
        //If single precision, get pointer as float
    	volatile float* coeffsPtr = (float*)&mandelbrot_base_ptr[MANDELBROT_COEFFS];
        //And store reduced precision value (square it as register expects zn^2)
        coeffsPtr[MANDELBROT_COEFF_ZNMAX(float)] = (float)(znMax * znMax);
    } else {
        //If double precision, get pointer as double
    	volatile double* coeffsPtr = (double*)&mandelbrot_base_ptr[MANDELBROT_COEFFS];
        //And store double value (square it as register expects zn^2)
        coeffsPtr[MANDELBROT_COEFF_ZNMAX(double)] = (znMax * znMax);
    }
    return MANDELBROT_SUCCESS;
}

//Set Coordinates
// - Sets coordinates and radius of pattern.
// - Change this to change the zoom.
signed int Mandelbrot_setCoordinates( double radius, double xcentre, double ycentre ) {
    double xsize = MANDELBROT_XSIZE(radius);
    double ysize = MANDELBROT_YSIZE(radius);
    double xmin = MANDELBROT_XMIN(xsize, xcentre);
    double ymin = MANDELBROT_YMIN(ysize, ycentre);
    double xstep = xsize / LT24_HEIGHT;
    double ystep = ysize / LT24_WIDTH;
    //Check if initialised
    if (!Mandelbrot_isInitialised()) return MANDELBROT_ERRORNOINIT;
    //Update internally
    mandelbrot_magnitude = radius;
    mandelbrot_xcentre = xcentre;
    mandelbrot_ycentre = ycentre;
    //Update device
    if (Mandelbrot_getCalculationPrecision() == MANDELBROT_FLOAT_PRECISION) {
        //If single precision, get pointer as float
        volatile float* coeffsPtr = (float*)&mandelbrot_base_ptr[MANDELBROT_COEFFS];
        //And store reduced precision values
        coeffsPtr[MANDELBROT_COEFF_XMIN(float)] = (float)xmin;
        coeffsPtr[MANDELBROT_COEFF_YMIN(float)] = (float)ymin;
        coeffsPtr[MANDELBROT_COEFF_XSTEP(float)] = (float)xstep;
        coeffsPtr[MANDELBROT_COEFF_YSTEP(float)] = (float)ystep;
    } else {
        //If double precision, get pointer as double
    	volatile double* coeffsPtr = (double*)&mandelbrot_base_ptr[MANDELBROT_COEFFS];
        //And store double values
        coeffsPtr[MANDELBROT_COEFF_XMIN(double)] = xmin;
        coeffsPtr[MANDELBROT_COEFF_YMIN(double)] = ymin;
        coeffsPtr[MANDELBROT_COEFF_XSTEP(double)] = xstep;
        coeffsPtr[MANDELBROT_COEFF_YSTEP(double)] = ystep;
    }
    return MANDELBROT_SUCCESS;
}

//Current iteration
// - Returns how many iterations have been made on the current pattern.
// - If >=0 is current iteration of this pattern
// - If <0 is error code
signed int Mandelbrot_currentIteration() {
    unsigned int iteration;
    if (!Mandelbrot_isInitialised()) return MANDELBROT_ERRORNOINIT;
    iteration = *((unsigned int*)&mandelbrot_base_ptr[MANDELBROT_ITERATION]);
    return (iteration & 0x7FFFFFFF); //Ensure the iteration value doesn't accidentally become error status code.
}

//Start new pattern
// - Resets the Mandelbrot generator and starts a new pattern.
// - Returns 0 if successfully started new pattern.
// - Ensure first that no iteration is currently running (Mandelbrot_iterationDone() returns SUCCESS).
signed int Mandelbrot_resetPattern() {
    //Check if driver initialised
    if (!Mandelbrot_isInitialised()) return MANDELBROT_ERRORNOINIT;
    //Check if generator is busy with existing iteration
    if (!(mandelbrot_base_ptr[MANDELBROT_FLAGS] & MANDELBROT_ITERATE)) {
        //If busy, don't allow reset:
        return MANDELBROT_BUSY;
        /* -- Reset not enabled, Mandelbrot IP core needs upgrading --
        //If busy, force reset
        mandelbrot_base_ptr[MANDELBROT_FLAGS] = MANDELBROT_RESET; //Reset generator
        while (!(mandelbrot_base_ptr[MANDELBROT_FLAGS] & MANDELBROT_ITERATE)); //Wait until reset complete.
        */
    }
    //Initialise the generator
    mandelbrot_base_ptr[MANDELBROT_FLAGS] = MANDELBROT_INIT; //Reset generator
    //Wait for it to be ready
    while (!(mandelbrot_base_ptr[MANDELBROT_FLAGS] & MANDELBROT_INIT)); //Wait until initialisation complete.
    //And done.
    return MANDELBROT_SUCCESS;
}

//Start next iteration
// - Return 0 if successfully started iteration.
// - Call mandelbrot_interationDone() to check if finished.
signed int Mandelbrot_startIteration() {
    signed int status = Mandelbrot_iterationDone();
    if (status != MANDELBROT_SUCCESS) return status;
    mandelbrot_base_ptr[MANDELBROT_FLAGS] = MANDELBROT_ITERATE; //Perform iteration.
    return MANDELBROT_SUCCESS;
}

//Check if last iteration is done
// - Return 0 if successfully finished iteration.
// - Return MANDELBROT_BUSY if still running.
signed int Mandelbrot_iterationDone() {
    unsigned char flags;
    //Check if driver initialised
    if (!Mandelbrot_isInitialised()) return MANDELBROT_ERRORNOINIT;
    //Check if generator is initialised
    flags = mandelbrot_base_ptr[MANDELBROT_FLAGS];
    if (!(flags & MANDELBROT_INIT)) return MANDELBROT_NOTREADY;
    //Check if generator is busy with existing iteration
    if (!(flags & MANDELBROT_ITERATE)) return MANDELBROT_BUSY;
    //Not busy if reached this far.
    return MANDELBROT_SUCCESS;
}

