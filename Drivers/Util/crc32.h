/* CRC32 Helper APIs
 * -----------------
 *
 * Provides an API for calculating the CRC32 checksum
 * of a data buffer, optionally with watchdog handling
 * for larger buffer sizes.
 * 
 * By default this just provides the APIs `crc32()` and
 * `crc32_wd()` (watchdog variant) required for tasks
 * such as decoding U-Boot UImage checksums. These APIs
 * are a wrapper which make use of a saved generic CRC
 * driver context.
 * 
 * It is expected that the user will call `crc32_setCtx()`
 * before using this driver, otherwise the checksum will 
 * always return as -1. The provided CRC driver context
 * must compute the standard CRC32 with settings:
 *   - Polynomial: 0x04C11DB7
 *   - Reverse Input/Output: true
 *   - Fixed initial value: false
 *   - Output XOR: 0  (*)
 * 
 * If `FAILBACK_SOFTWARE_CRC32` is defined for this files
 * compilation unit, a software based CRC32 calculation
 * routine is included as a failback when the CRC driver
 * context has not been set.
 * 
 * (*) XOR with 0xFFFFFFFF is applied by the crc32 API
 * so should not be performed by the CRC driver.
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

#ifndef CRC32_H_
#define CRC32_H_

#include "Util/driver_crc.h"

// Set the CRC processor context used by the CRC32 routines.
// - If this returns an error code, the CRC32 functions will not work.
HpsErr_t crc32_setCtx(PCRCCtx_t ctx);

// CRC32 calculation for small buffers
uint32_t crc32(uint32_t crc, const uint8_t *p, uint32_t len);

// CRC32 calculation for large buffers
// - Breaks the calculation into chunks interspersed with watchdog resets
uint32_t crc32_wd(uint32_t crc, const uint8_t *buf, uint32_t len, uint32_t chunk_sz);


#endif /* CRC32_H_ */
