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
#include <stdbool.h> //Boolean variable type "bool" and "true"/"false" constants.

//Error Codes
#define MANDELBROT_SUCCESS       0
#define MANDELBROT_ERRORNOINIT  -1
#define MANDELBROT_BUSY         -3
#define MANDELBROT_NOTREADY     -7

//Precision
#define MANDELBROT_DOUBLE_PRECISION true
#define MANDELBROT_FLOAT_PRECISION false


//Function to initialise the Mandelbrot driver
// - Requires that LT24_initialise has been called already.
// - Returns 0 if successful
signed int Mandelbrot_initialise( unsigned int mandelbrot_base_address );

//Check if driver initialised
// - returns true if initialised
bool Mandelbrot_isInitialised( void );

//Get Precision
// - returns true if double precision
bool Mandelbrot_getCalculationPrecision( void );

//Set Precision
// - Default to FLOAT
// - Sets the precision of the controller
// - returns 0 if successful
// - Once precision is changed, all coefficients (Zmax, Xmin, Ymin, Xstep, Ystep)
//   will automatically be reprogrammed.
signed int Mandelbrot_setCalculationPrecision( bool doublePrecision );

//Set Bounding Value
// - This typically never needs changing. It defaults to 2.
signed int Mandelbrot_setZnMax( double znMax );

//Set Coordinates
// - Sets coordinates and radius of pattern.
// - Change this to change the zoom.
signed int Mandelbrot_setCoordinates( double radius, double xcentre, double ycentre );

//Current iteration
// - Returns how many iterations have been made on the current pattern.
// - If >=0 is current iteration of this pattern
// - If <0 is error code
signed int Mandelbrot_currentIteration( void );

//Start new pattern
// - Resets the Mandelbrot generator and starts a new pattern.
// - Returns 0 if successfully started new pattern.
// - Ensure first that no iteration is currently running (Mandelbrot_iterationDone() returns SUCCESS).
signed int Mandelbrot_resetPattern( void );

//Start next iteration
// - Return 0 if successfully started iteration.
// - Call mandelbrot_interationDone() to check if finished.
signed int Mandelbrot_startIteration( void );

//Check if last iteration is done
// - Return 0 if successfully finished iteration.
// - Return MANDELBROT_BUSY if still running.
signed int Mandelbrot_iterationDone( void );

#endif /* DE1SOC_MANDELBROT_H_ */
