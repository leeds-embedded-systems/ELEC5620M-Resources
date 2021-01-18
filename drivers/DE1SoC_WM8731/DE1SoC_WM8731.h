/* 
 * WM8731 Audio Controller Driver
 * ------------------------------
 * Description: 
 * Driver for the WM8731 Audio Controller
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+-------------------------------
 * 20/09/2017 | Creation of driver
 * 20/10/2017 | Change to include status codes
 *
 */

#ifndef DE1SoC_WM8731_H_
#define DE1SoC_WM8731_H_

//Include required header files
#include <stdbool.h> //Boolean variable type "bool" and "true"/"false" constants.

//Error Codes
#define WM8731_SUCCESS      0
#define WM8731_ERRORNOINIT -1

//FIFO Space Offsets
#define WM8731_RARC 0
#define WM8731_RALC 1
#define WM8731_WSRC 2
#define WM8731_WSLC 3

//Initialise Audio Codec
// - base_address is memory-mapped address of audio controller
// - returns 0 if successful
signed int WM8731_initialise ( unsigned int base_address );

//Check if driver initialised
// - Returns true if driver previously initialised
// - WM8731_initialise() must be called if false.
bool WM8731_isInitialised( void );

//Clears FIFOs
// - returns 0 if successful
signed int WM8731_clearFIFO( bool adc, bool dac);

//Get FIFO Space Address
volatile unsigned char* WM8731_getFIFOSpacePtr( void );

//Get Left FIFO Address
volatile unsigned int* WM8731_getLeftFIFOPtr( void );

//Get Right FIFO Address
volatile unsigned int* WM8731_getRightFIFOPtr( void );

#endif /*DE1SoC_WM8731_H_*/
