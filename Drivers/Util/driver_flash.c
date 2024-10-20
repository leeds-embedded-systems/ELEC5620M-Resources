/* Generic Flash Driver Interface
 * -------------------------------
 *
 * Provides a common interface for different Flash
 * drivers to allow them to be used as a generic
 * handler.
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 12/10/2024 | Add read-only/write-protect APIs.
 * 01/01/2024 | Creation of driver.
 */

#include "driver_flash.h"

bool FLASH_rangeInRegion(PFlashRegion_t region, unsigned int address, unsigned int length) {
    if (!region || !region->valid) return false;
    // Calculate end of range
    unsigned int addressEnd = address + length - 1;
    // Don't allow wrap around or zero length region
    if ((length == 0) || (addressEnd < address)) return false;
    // Return true if full requested address range is within this region
    return ((address >= region->start) && (addressEnd <= region->end));
}


