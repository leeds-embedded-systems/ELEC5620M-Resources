/*
 * Common Driver Error Codes
 * -------------------------
 *
 * Provides a unified set of error codes used by drivers
 *
 * This file should not be modified as it is shared across
 * many different projects.
 * 
 * 
 * Custom Error Codes
 * ------------------
 * 
 * To add custom error codes, make your own enumeration and
 * start them at ERR_CUSTOM_OFF getting more negative. Prefix
 * any custom error codes with ERR_CUSTOM_ not ERR_ to avoid
 * future compatibility issues. The largest allowed custom
 * error code is (ERR_CUSTOM_OFS - 16384) to prevent overflow
 * on systems which use a 16-bit int. 
 * 
 * For example:
 * 
 *     enum {
 *        ERR_CUSTOM_MYERROR = ERR_CUSTOM_OFS,
 *        ERR_CUSTOM_ANOTHER = ERR_CUSTOM_OFS - 1,
 *        ... etc ...
 *     };
 * 
 * 
 * Conversion to String
 * --------------------
 * 
 * To convert to string, use the following which returns
 * a const char*:
 * 
 *     const char* errStr = enumToStringSafe(status, EnumLookupTableAndSize(ErrCodes));
 * 
 * This can be piped in to printf, e.g.:
 * 
 *     printf("Oh no! Error %s (%d) occurred!", errStr, status);
 * 
 * You must globally define ENUM_LOOKUP_ENABLED to use the
 * lookup feature.
 * 
 * Custom error codes do not support char* lookup unless you
 * make your enum using the macro style used for the main
 * error enum, and manually check if the value is <= the
 * custom offset to select your own lookup table.
 * 
 * 
 * 
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 18/02/2024 | Add error code string lookup
 * 28/12/2023 | Creation of error code header
 *
 */

#ifndef UTIL_ERROR_H_
#define UTIL_ERROR_H_

#include "enum_lookup.h"

// Default error check scheme:
typedef signed int HpsErr_t;
// Success only if ERR_SUCCESS, error otherwise
#define IS_SUCCESS(code)   ((code) == ERR_SUCCESS)
#define IS_ERROR(code)     !IS_SUCCESS(code)
#define IS_BUSY(code)      ((code) == ERR_BUSY)
#define IS_RETRY(code)     ((code) == ERR_AGAIN)
#define IS_NOSUPPORT(code) ((code) == ERR_NOSUPPORT)

// Extended error check scheme:
typedef signed int HpsErrExt_t;
// Success if >= ERR_SUCCESS, error otherwise
#define IS_SUCCESS_EXT(code) ((code) >= ERR_SUCCESS)
#define IS_ERROR_EXT(code) !IS_SUCCESS_EXT(code)
#define IS_TRUE_EXT(code) ((code) > ERR_SUCCESS) // Check if HpsErrExt_t is boolean true

// Error Codes
// This uses a special macro to generate the enum values to enable providing
// a string lookup of error codes for debug or display purposes.
// Ensure all lines end with a backslash (\)
#define ERROR_CODES_LIST(m, type)                                                                      \
    /* Default success status. */                                                                      \
    m(,ERR_SUCCESS   ,,=  0)                                                                           \
                                                                                                       \
    /* Initialisation failed or not performed */                                                       \
    m(,ERR_NOINIT    ,,= -1)                                                                           \
    m(,ERR_PARTIAL   ,,= -2)                                                                           \
    m(,ERR_UNKNOWN   ,,= -3)                                                                           \
                                                                                                       \
    /* Pointers */                                                                                     \
    m(,ERR_NULLPTR   ,,= -10)                                                                          \
    m(,ERR_ALIGNMENT ,,= -11)                                                                          \
    m(,ERR_BADID     ,,= -12)                                                                          \
                                                                                                       \
    /* Array access */                                                                                 \
    m(,ERR_ALLOCFAIL ,,= -20)                                                                          \
    m(,ERR_NOSPACE   ,,= -21)                                                                          \
    m(,ERR_BEYONDEND ,,= -22)                                                                          \
    m(,ERR_NOTFOUND  ,,= -23)                                                                          \
    m(,ERR_ISEMPTY   ,,= -24)                                                                          \
    m(,ERR_REVERSED  ,,= -25)                                                                          \
                                                                                                       \
    /* Timed out */                                                                                    \
    m(,ERR_BUSY      ,,= -30)                                                                          \
    m(,ERR_TIMEOUT   ,,= -31)                                                                          \
    m(,ERR_ABORTED   ,,= -32)                                                                          \
    m(,ERR_AGAIN     ,,= -33)  /* Use only if a function must be called again to check some result. */ \
    m(,ERR_SKIPPED   ,,= -34)                                                                          \
                                                                                                       \
    /* Device error */                                                                                 \
    m(,ERR_BADDEVICE ,,= -40)                                                                          \
    m(,ERR_NOCONNECT ,,= -41)                                                                          \
    m(,ERR_INUSE     ,,= -42)                                                                          \
    m(,ERR_NOSUPPORT ,,= -43)                                                                          \
    m(,ERR_NOTREADY  ,,= -44)                                                                          \
    m(,ERR_WRONGMODE ,,= -45)                                                                          \
                                                                                                       \
    /* Values */                                                                                       \
    m(,ERR_TOOBIG    ,,= -50)                                                                          \
    m(,ERR_TOOSMALL  ,,= -51)                                                                          \
    m(,ERR_CHECKSUM  ,,= -52)                                                                          \
    m(,ERR_MISMATCH  ,,= -53)                                                                          \
    m(,ERR_CORRUPT   ,,= -54)                                                                          \
    m(,ERR_OUTRANGE  ,,= -55)                                                                          \
                                                                                                       \
    /* File/Disk */                                                                                    \
    m(,ERR_WRITEPROT ,,= -80)                                                                          \
    m(,ERR_BADDISK   ,,= -81)                                                                          \
    m(,ERR_IOFAIL    ,,= -82)                                                                          \
                                                                                                       \
    /* User Custom Error Codes Offset */                                                               \
    m(,ERR_CUSTOM_OFS,,= -16384)

typedef enum {
    ERROR_CODES_LIST(GENERATE_ENUM, ErrCodes)
} ErrCodes;
GENERATE_ENUM_LOOKUP_TABLE_HEADER(ErrCodes, ERROR_CODES_LIST);


#endif /* UTIL_ERROR_H_ */
