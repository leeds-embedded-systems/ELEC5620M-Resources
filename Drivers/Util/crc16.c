/* CRC16 Helper APIs
 * -----------------
 *
 * Provides an API for calculating the CRC16 checksum
 * of a data buffer, optionally with watchdog handling
 * for larger buffer sizes.
 * 
 * By default this just provides the APIs `crc16()`
 * required for tasks such as the UARP RS485 comms.
 * These APIs are a wrapper which make use of a saved
 * generic CRC driver context.
 * 
 * It is expected that the user will call `crc16_setCtx()`
 * before using this driver, otherwise the checksum will 
 * always return as -1. The provided CRC driver context
 * must compute the standard CRC16-CITT with settings:
 *   - Polynomial: 0x8005
 *   - Reverse Input/Output: true
 *   - Fixed initial value: true
 *   - Output XOR: 0
 * 
 * If `FAILBACK_SOFTWARE_CRC16` is defined for this files
 * compilation unit, a software based CRC16 calculation
 * routine is included as a failback when the CRC driver
 * context has not been set.
 * 
 * When using NIOS2, a CRC custom instruction can be used
 * instead. If this is used, globally define the name
 * of the instruction as NIOS2_INSTRUCTION_CRC16. For example
 * if ALT_CI_CRC_INSTRUCTION(n,A) is the instruction macro
 * in "system.h", then define:
 *    NIOS2_INSTRUCTION_CRC16=ALT_CI_CRC_INSTRUCTION
 * 
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 07/07/2024 | Creation of driver.
 */

#include "crc16.h"

#include "Util/bit_helpers.h"

/*
 * Internal APIs
 */

#define CRC_NAME            "CRC-16"
#define POLYNOMIAL          0x8005
#define INITIAL_REMAINDER   0x0000
#define FINAL_XOR_VALUE     0x0000
#define REFLECT_DATA        TRUE
#define REFLECT_REMAINDER   TRUE
#define WIDTH               16
#define MSB                 (1 << (WIDTH - 1))

#ifdef FAILBACK_SOFTWARE_CRC16

uint16_t _crc16_failback(const uint8_t * data, uint32_t length) {
    uint16_t remainder = INITIAL_REMAINDER;
    uint32_t byte;
    uint32_t bit;

    /*
     * Perform modulo-2 division, a byte at a time.
     */
    for (byte = 0; byte < length; byte++) {
#if REFLECT_DATA
        /*
         * Reflect the data about the centre bit. Then align at top of WIDTH bits.
         */
        uint16_t dataByte = (uint16_t)((uint32_t)__rbit((uint32_t )data[byte]) >> (32 - WIDTH));
#else
        /*
         * Extract data and align at top of WIDTH bits. (Currently at bottom, so shift up)
         */
        uint16_t dataByte = (uint16_t)((uint16_t)data[byte] << (WIDTH - 8));
#endif
        /*
         * Bring the next byte into the remainder.
         */
        remainder ^= dataByte;

        /*
         * Perform modulo-2 division, a bit at a time.
         */
        for (bit = 8; bit > 0; bit--) {
            /*
             * Try to divide the current data bit.
             */
            if (remainder & MSB) {
                remainder = (remainder << 1) ^ POLYNOMIAL;
            } else {
                remainder = (remainder << 1);
            }
        }
    }

#if REFLECT_REMAINDER
    /*
     * Reflect the data about the centre bit. Then align at top of WIDTH bits.
     */
    remainder = (uint16_t)((uint32_t)__rbit((uint32_t )remainder) >> (32 - WIDTH));
#endif
    /*
     * The final remainder is the CRC result.
     */
    return (remainder ^ FINAL_XOR_VALUE);
}

#endif

#ifndef NIOS2_INSTRUCTION_CRC16
static CRCCtx_t* _crc16Proc = NULL;

// Base CRC16 function for driver calls
static uint16_t _crc16_driver(const uint8_t *p, uint32_t len) {
	unsigned int crc = INITIAL_REMAINDER;
    HpsErr_t status = CRC_calculate(_crc16Proc, true, p, len, &crc);
    return ERR_IS_ERROR(status) ? (uint16_t)-1 : (((uint16_t)crc) ^ FINAL_XOR_VALUE);
}
#else

#ifndef __NIOS2__
#error Must be NIOS2 to use NIOS2 custom instruction
#endif

#include <system.h>

#define CRC16_CI NIOS2_INSTRUCTION_CRC16

/*The n values and their corresponding operation are as follow:
 * n = 0, Initialise the custom instruction to the initial remainder value
 * n = 1, Write  8 bits data to custom instruction
 * n = 2, Write 16 bits data to custom instruction
 * n = 3, Write 32 bits data to custom instruction
 * n = 4, Read  32 bits data from the custom instruction
 * n = 5, Read  64 bits data from the custom instruction
 * n = 6, Read  96 bits data from the custom instruction
 * n = 7, Read 128 bits data from the custom instruction*/

static uint16_t _crc16_driver(const uint8_t *p, uint32_t len) {

    /* Ensure correct alignment */
    if (!pointerIsAligned(p,   sizeof(uint32_t)) ||
    	!addressIsAligned(len, sizeof(uint32_t))
    ) {
        return UINT16_MAX;  
    } 

    /* The custom instruction CRC will initialise to the initial remainder value */
    CRC16_CI(0,0);

    /* Convert length to 32bit words */
    uint32_t length = len / sizeof(uint32_t);

    /* Write 32 bit data to the custom instruction. The input buffer is made of
     * 32bit numbers, so we never have to worry about odd boundaries.
     */
    uint32_t* data = (uint32_t*)p;
    while (length--) {
        CRC16_CI(3, *data++);
    }

    /* There are 4 registers in the CRC custom instruction.  Since
     * this function uses CRC-16 only the first register must be read
     * in order to receive the full result.
     */
    return (uint16_t)CRC16_CI(4, 0);
}

#endif

/*
 * User Facing APIs
 */


#ifndef NIOS2_INSTRUCTION_CRC16

// Set the CRC processor context used by the CRC16 routines.
// - If this returns an error code, the CRC16 functions will not work.
// - Passing a null pointer as crcCtx will clear the current handler.
//   This should be done if the crc driver being used as the current
//   handler is cleaned up.
HpsErr_t crc16_setCtx(CRCCtx_t* crcCtx) {
    _crc16Proc = NULL;
    if (!crcCtx) return ERR_SUCCESS;
    // Must be an initialised CRC object
    if (!CRC_isInitialised(crcCtx)) return ERR_NOINIT;
    // Must be 16bit CRC
    HpsErr_t width = CRC_getWidth(crcCtx);
    if (width != 16) return ERR_IS_ERROR(width) ? (HpsErr_t)width : ERR_MISMATCH;
    // Save context and return status
    _crc16Proc = crcCtx;
    return ERR_SUCCESS;
}

// Returns true if the CRC16 context is valid
bool crc16_checkCtx() {
#ifdef FAILBACK_SOFTWARE_CRC16
    return true; // Has software failback, so always ready
#else
    return CRC_isInitialised(_crc16Proc); // Only ready if context is valid
#endif
}

#else

// Returns true if the CRC16 context is valid
bool crc16_checkCtx() {
    return true; // Has NIOS CI, so always ready
}

#endif

// CRC16 calculation for small buffers
uint16_t crc16(const uint8_t *p, uint32_t len) {
#ifdef FAILBACK_SOFTWARE_CRC16
    if (!CRC_isInitialised(_crc16Proc))
        return _crc16_failback(p, len);
    else
#endif
        return _crc16_driver(p, len);
}


