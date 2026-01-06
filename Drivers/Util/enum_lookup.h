/*
 * Enum with String Lookup Generator
 * ---------------------------------
 *
 * Allows genenrating typed enumerations using
 * a macro to define the entries and values. This
 * same macro can then be used to generate a string
 * lookup table for the enum values.
 * 
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 27/10/2025 | Add kernel support for Lnx/Win
 *            | Trim leading/trailing whitespace in lookup.
 * 30/01/2024 | Adapt for embedded code from DLL
 * 30/01/2018 | Creation of utility
 *
 */

#ifndef ENUMLOOKUP_H_
#define ENUMLOOKUP_H_

#if defined(_WIN32)
//Include windows header
#ifdef _KERNEL_MODE
#include <ntddk.h>
#else
#include <windows.h>
#endif
#elif defined( __linux__ ) || defined(__arm__) || defined(__arm_on_nios__)
 //Include standard libraries
#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/ctype.h>
#include <linux/string.h>
#else
#include <stdint.h>
#include <stddef.h>
#include <ctype.h>
#include <string.h>
#endif
#elif defined(__NIOS2__) || defined(__AVR__)
 //For Nios/Arm HPS/AVR, include the UsefulDefines.h header which has everything we need.
#include "UsefulDefines.h"
#include <string.h>
#else
#error Unknown System
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Useful defines for Enum/String generation
#define   GENERATE_ENUM(PFX,NAME,SFX,VAL)   PFX ##  NAME ##  SFX VAL,
#define GENERATE_STRING(PFX,NAME,SFX,...)  #PFX    #NAME    #SFX,
#define GENERATE_LOOKUP(PFX,NAME,SFX,...) {#PFX    #NAME    #SFX, (size_t) PFX ## NAME ## SFX},

#define UNKNOWN_STR "???"

typedef struct {
    const char* str;
    size_t      enumVal;
} EnumLookupTable_t;

#if defined(ENUM_LOOKUP_ENABLED) || defined(PCIEUARPLIBSYNC_EXPORTS)

//Convert an enum to string.
// - Returns NULL if not found
const char* enumToString(size_t enumVal, const EnumLookupTable_t* lookupTable, size_t lookupTableLength);

//Convert an enum to string.
// - Retruns `UNKNWONW_STR` if not found
const char* enumToStringSafe(size_t enumVal, const EnumLookupTable_t* lookupTable, size_t lookupTableLength);

//Convert a string to an enum value
// - Ignores leading and trailing whitespace in `str`.
// - Returns `notFoundVal` if not found.
size_t stringToEnum(const char* str, const EnumLookupTable_t* lookupTable, size_t lookupTableLength, size_t notFoundValue);

//Pass this to above functions to provide arguments 2 and 3 for a given enum type.
#define EnumLookupTableAndSize(enumType) enumType##_Lookup, enumType##_Lookup_Length

// Generate lookup table header file
#define GENERATE_ENUM_LOOKUP_TABLE_HEADER(enumType, lookupMacro) \
extern const EnumLookupTable_t enumType##_Lookup [];   \
extern const size_t enumType##_Lookup_Length

// Generate lookup table header file
#define GENERATE_ENUM_LOOKUP_TABLE_SOURCE(enumType, lookupMacro) \
const EnumLookupTable_t enumType##_Lookup [] = {  \
    lookupMacro(GENERATE_LOOKUP,enumType) \
};                                        \
const size_t enumType##_Lookup_Length = sizeof(enumType##_Lookup) / sizeof(EnumLookupTable_t)

#else
// Otherwise don't generate the lookups if not needed.
#define enumToString(...)     (NULL)
#define enumToStringSafe(...) UNKNOWN_STR
#define stringToEnum(...)     (-1)

#define GENERATE_ENUM_LOOKUP_TABLE_HEADER(...)
#define GENERATE_ENUM_LOOKUP_TABLE_SOURCE(...)

#endif

#ifdef __cplusplus
}
#endif

#endif //ENUMLOOKUP_H_
