/*
 * HPS I2C Driver
 * ------------------------------
 * Description:
 * Driver for the HPS embedded I2C controller
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+-----------------------------------
 * 20/09/2017 | Creation of driver
 * 20/10/2017 | Change to include status codes
 * 13/07/2019 | Support Controller ID 1 (LTC Hdr)
 *
 */

#ifndef HPS_I2C_H_
#define HPS_I2C_H_

#include <stdbool.h>

//Error Codes
#define HPS_I2C_SUCCESS      0
#define HPS_I2C_ERRORNOINIT -1
#define HPS_I2C_INVALIDID   -2
#define HPS_I2C_BUSY        -3
#define HPS_I2C_INVALIDLEN  -4
#define HPS_I2C_ABORTED     -5

//Initialise HPS I2C Controller
// - controller is id of the I2C controller to initialised.
// - DE1-SoC uses ID 0 for Accelerometer/VGA/Audio/ADC. ID 1 for LTC 14pin Hdr.
// - Returns 0 if successful.
signed int HPS_I2C_initialise(unsigned int controller_id);

//Check if driver initialised
// - controller is id of the I2C controller to initialised.
// - Returns true if driver previously initialised
// - HPS_I2C_init(controller_id) must be called if false.
bool HPS_I2C_isInitialised(unsigned int controller_id);

//Functions to write data
// - controller is id of the I2C controller to initialised.
// - 7bit address is I2C slave device address
// - data is data to be sent (8bit, 16bit, 32bit or array respectively)
// - Returns 0 if successful.
signed int HPS_I2C_write8b(unsigned int controller_id, unsigned char address, unsigned char data);
signed int HPS_I2C_write16b(unsigned int controller_id, unsigned char address, unsigned short data);
signed int HPS_I2C_write32b(unsigned int controller_id, unsigned char address, unsigned int data);
signed int HPS_I2C_write(unsigned int controller_id, unsigned char address, unsigned char data[], unsigned int length);

#endif /* HPS_I2C_H_ */
