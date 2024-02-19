/*
 * Common Driver Error Codes
 * -------------------------
 *
 * Provides a unified set of error codes used by drivers
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 18/02/2024 | Add error code string lookup
 *
 */

#include "error.h"

GENERATE_ENUM_LOOKUP_TABLE_SOURCE(ErrCodes, ERROR_CODES_LIST);
