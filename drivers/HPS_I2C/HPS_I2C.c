
#include "HPS_I2C.h"
#include "../HPS_Watchdog/HPS_Watchdog.h"

#include <math.h>

//I2C Controller Base Addresses
volatile unsigned int *i2c_base_ptr[2] = {(unsigned int *)0xFFC04000,(unsigned int *)0xFFC05000};

//Driver global static variables (visible only to this .c file)
bool i2c_initialised[2] = {false,false};

#define HPS_I2C_CON	    (0x00/sizeof(unsigned int))
#define HPS_I2C_TAR     (0x04/sizeof(unsigned int))
#define HPS_I2C_DATCMD  (0x10/sizeof(unsigned int))
#define HPS_I2C_SSHCNT  (0x14/sizeof(unsigned int))
#define HPS_I2C_SSLCNT  (0x18/sizeof(unsigned int))
#define HPS_I2C_FSHCNT  (0x1C/sizeof(unsigned int))
#define HPS_I2C_FSLCNT  (0x20/sizeof(unsigned int))
#define HPS_I2C_IRQFLG  (0x2C/sizeof(unsigned int))
#define HPS_I2C_CLRTXA  (0x54/sizeof(unsigned int))
#define HPS_I2C_ENABLE  (0x6C/sizeof(unsigned int))
#define HPS_I2C_STATUS  (0x70/sizeof(unsigned int))

//Initialise HPS I2C Controller
// - controller is id of the I2C controller to initialised.
// - DE1-SoC uses ID 0 for Accelerometer/VGA/Audio/ADC. ID 1 for LTC 14pin Hdr.
// - Returns true if successful.
signed int HPS_I2C_initialise(unsigned int controller_id){
	//Local variables
    unsigned int* gpio1_base_ptr = (unsigned int*) 0xFF709000;
    //Calculate I2C clock high/low parameters

    //Check if valid I2C controller id.
    if (controller_id > 1) return HPS_I2C_INVALIDID; //invalid id.

    //Ensure I2C disabled for configuration
    i2c_base_ptr[controller_id][HPS_I2C_ENABLE] = 0x00;

    //If I2C controller ID 0, make sure GPIO is configured to route external I2C mux on DE1-SoC to HPS
    if (controller_id == 0) {
        gpio1_base_ptr[1] = gpio1_base_ptr[1] | (1 << 19); //Make sure bit 19 (GPIO48) is an output
        gpio1_base_ptr[0] = gpio1_base_ptr[0] | (1 << 19); //Then set it high.
    }
    
    //If I2C controller ID 1, make sure GPIO is configured to route LTC Header I2C mux on DE1-SoC to HPS
    if (controller_id == 1) {
        gpio1_base_ptr[1] = gpio1_base_ptr[1] | (1 << 11); //Make sure bit 11 (GPIO40) is an output
        gpio1_base_ptr[0] = gpio1_base_ptr[0] | (1 << 11); //Then set it high.
    }
    
    //Configure the I2C peripheral
    i2c_base_ptr[controller_id][HPS_I2C_CON   ] = 0x65;     //I2C master mode, 400kHz, 7-bit address
    //See "Hard Processor System Technical Reference Manual" section 20 for magic calculations of HCNT/LCNT
    i2c_base_ptr[controller_id][HPS_I2C_FSHCNT] = 60; //I2C clock high parameter for 400kHz
    i2c_base_ptr[controller_id][HPS_I2C_FSLCNT] = 130; //I2C clock low parameter for 400kHz

    //Enable the I2C peripheral
    i2c_base_ptr[controller_id][HPS_I2C_ENABLE] = 0x01;     //I2C enabled

    //Now initialised
    i2c_initialised[controller_id] = true;
    return HPS_I2C_SUCCESS; //success
}



//Check if driver initialised
// - controller is id of the I2C controller to initialised.
// - Returns true if driver previously initialised
// - HPS_I2C_init(controller_id) must be called if false.
bool HPS_I2C_isInitialised(unsigned int controller_id){
    if (controller_id > 1) return false; //invalid id.
    return i2c_initialised[controller_id];
}


//Functions to write data
// - controller is id of the I2C controller to initialised.
// - address is I2C slave device address
// - data is data to be sent (8bit, 16bit, 32bit or array respectively)
// - Returns true if successful.
signed int HPS_I2C_write8b(unsigned int controller_id, unsigned char address, unsigned char data){
    return HPS_I2C_write(controller_id, address, &data, 1);
}
signed int HPS_I2C_write16b(unsigned int controller_id, unsigned char address, unsigned short data){
    //Remap data to big-endian
    unsigned char* dataptr = (unsigned char *)&data;
    unsigned char data_arr[2];
    data_arr[0] = dataptr[1];
    data_arr[1] = dataptr[0];
    //Send data as array
    return HPS_I2C_write(controller_id, address, data_arr, sizeof(data_arr));
}
signed int HPS_I2C_write32b(unsigned int controller_id, unsigned char address, unsigned int data){
    //Remap data to big-endian
    unsigned char* dataptr = (unsigned char *)&data;
    unsigned char data_arr[4];
    data_arr[0] = dataptr[3];
    data_arr[1] = dataptr[2];
    data_arr[2] = dataptr[1];
    data_arr[3] = dataptr[0];
    //Send data as array
    return HPS_I2C_write(controller_id, address, data_arr, sizeof(data_arr));
}
signed int HPS_I2C_write(unsigned int controller_id, unsigned char address, unsigned char data[], unsigned int length){
    //Validate write request
    if (controller_id > 1) return HPS_I2C_INVALIDID; //invalid id.
    if (!HPS_I2C_isInitialised(controller_id)) return HPS_I2C_ERRORNOINIT; //not initialised
    if (i2c_base_ptr[controller_id][HPS_I2C_STATUS] & (1 << 5)) return HPS_I2C_BUSY; //I2C master busy
    if (length == 0) return HPS_I2C_INVALIDLEN; //invalid length. Must send at least one byte.
    if (length > 255) return HPS_I2C_INVALIDLEN; //Transfer too long. FIFO has space for 1 address and 255 data words. Could add throttling later.
    //If a valid controller and the master is not currently busy, perform our transfer
    i2c_base_ptr[controller_id][HPS_I2C_TAR] = address; //Load the target address as 7bit address in master mode
    //Load the transmit data into the TX FIFO
    do {
        unsigned int datcmd = (unsigned int)*data++; //Get the next data and pad to unsigned int.
        if (length == 1) { //If this is the last word
            datcmd |= (1<<9); //Set the "stop" bit of the datcmd register so controller issues end of I2C transaction
        }
        i2c_base_ptr[controller_id][HPS_I2C_DATCMD] = datcmd;
    } while (--length); //Repeat while there is more data to send
    //Wait for master to finish
    while (i2c_base_ptr[controller_id][HPS_I2C_STATUS] & (1 << 5)) {
        //Check for a TX abort IRQ
        if (i2c_base_ptr[controller_id][HPS_I2C_IRQFLG] & (1 << 6)){
            i2c_base_ptr[controller_id][HPS_I2C_CLRTXA] = 1; //Clear TX abort flag.
            return HPS_I2C_ABORTED; //Aborted if any of the abort flags are set.
        }
        ResetWDT();
    }
    //And done.
    return HPS_I2C_SUCCESS; //success
}
