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
 * 28/12/2023 | Creation of error code header
 *
 */

#ifndef UTIL_ERROR_H_
#define UTIL_ERROR_H_


// Default error check scheme:
typedef signed int HpsErr_t;
// Success only if ERR_SUCCESS, error otherwise
#define IS_SUCCESS(code) ((code) == ERR_SUCCESS)
#define IS_ERROR(code)   !IS_SUCCESS(code)
#define IS_BUSY(code)    ((code) == ERR_BUSY)
#define IS_RETRY(code)   ((code) == ERR_AGAIN)

// Extended error check scheme:
typedef signed int HpsErrExt_t;
// Success if >= ERR_SUCCESS, error otherwise
#define IS_SUCCESS_EXT(code) ((code) >= ERR_SUCCESS)
#define IS_ERROR_EXT(code) !IS_SUCCESS_EXT(code)
#define IS_TRUE_EXT(code) ((code) > ERR_SUCCESS) // Check if HpsErrExt_t is boolean true

// Error Codes
enum {
    // Default success status.
    ERR_SUCCESS   =  0,

    // Initialisation failed or not performed
    ERR_NOINIT    = -1,
    ERR_PARTIAL   = -2,

    // Pointers
    ERR_NULLPTR   = -10,
    ERR_ALIGNMENT = -11,
    ERR_BADID     = -12,

    // Array access
    ERR_ALLOCFAIL = -20,
    ERR_NOSPACE   = -21,
    ERR_BEYONDEND = -22,
    ERR_NOTFOUND  = -23,
    ERR_ISEMPTY   = -24,
    ERR_REVERSED  = -25,

    // Timed out
    ERR_BUSY      = -30,
    ERR_TIMEOUT   = -31,
    ERR_ABORTED   = -32,
    ERR_AGAIN     = -33,  // Use only if a function must be called again to check some result.
    ERR_SKIPPED   = -34,

    // Device error
    ERR_BADDEVICE = -40,
    ERR_NOCONNECT = -41,
    ERR_INUSE     = -42,
    ERR_NOSUPPORT = -43,
    ERR_NOTREADY  = -44,
    ERR_WRONGMODE = -45,

    // Values
    ERR_TOOBIG    = -50,
    ERR_TOOSMALL  = -51,
    ERR_CHECKSUM  = -52,
    ERR_MISMATCH  = -53,
    ERR_CORRUPT   = -54,

    // File/Disk
    ERR_WRITEPROT = -80,
    ERR_BADDISK   = -81,
    ERR_IOFAIL    = -82
};

#endif /* UTIL_ERROR_H_ */
