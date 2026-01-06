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
 *   - Fixed initial value: optional
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

#ifndef CRC16_H_
#define CRC16_H_

#include "Util/driver_crc.h"

#ifndef NIOS2_INSTRUCTION_CRC16
// Set the CRC processor context used by the CRC16 routines.
// - If this returns an error code, the CRC16 functions will not work.
HpsErr_t crc16_setCtx(CRCCtx_t* ctx);
#endif

// Returns true if the CRC16 context is valid
bool crc16_checkCtx();

// CRC16 calculation for small buffers
// - Has fixed initial remainder so can't be chained.
uint16_t crc16(const uint8_t *p, uint32_t len);


#endif /* CRC16_H_ */
