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

#if defined(_WIN32)
    #include "EnumLookup.h"
    #include "DebuggerOutput.h"
    #ifdef _KERNEL_MODE
        #include "Public.h"
        #define DISABLE_ENUM_LOOKUP_MATCH_LOG
        #define EnumDbgPrintEx(...) DrvDbgPrintEx(LEVEL_WARN,__VA_ARGS__)
    #else
        #define EnumDbgPrintEx(...) DLLDbgPrintEx(__VA_ARGS__)
    #endif
    #define ENUM_VAL_FMT "%zd"
#elif defined( __linux__ )
    #include "EnumLookup.h"
    #include "DebuggerOutput.h"
    #ifdef __KERNEL__
        #include "Public.h"
        #define DISABLE_ENUM_LOOKUP_MATCH_LOG
        #define EnumDbgPrintEx(...) DrvDbgPrintEx(LEVEL_WARN,__VA_ARGS__)
    #else
        #define EnumDbgPrintEx(...) DLLDbgPrintEx(__VA_ARGS__)
    #endif
    #define ENUM_VAL_FMT "%ld"
#elif defined(__arm__) || defined(__arm_on_nios__)
    #include "enum_lookup.h"
    #include "Util/verbosity.h"
    #ifndef DEBUG
        #define EnumDbgPrintEx(...) DbgPrintf(VERBOSE_EXTRAINFO,__VA_ARGS__)
    #else
        #define EnumDbgPrintEx(...) printf(__VA_ARGS__)
    #endif
    #define ENUM_VAL_FMT "%d"
#else
    #include "EnumLookup.h"
    #define ENUM_VAL_FMT "%d"
#endif

#ifndef EnumDbgPrintEx
#define EnumDbgPrintEx(...) ((void)0)
#endif

#ifndef enumToString
const char* enumToString(size_t enumVal, const EnumLookupTable_t* lookupTable, size_t lookupTableLength) {
    size_t idx;
    for (idx = 0; idx < lookupTableLength; idx++) {
        if (enumVal == lookupTable[idx].enumVal) {
#ifndef DISABLE_ENUM_LOOKUP_MATCH_LOG
            EnumDbgPrintEx("Matched enumVal " ENUM_VAL_FMT " to string %s\n", enumVal, lookupTable[idx].str);
#endif
            return lookupTable[idx].str;
        }
    }
    EnumDbgPrintEx("Could not match enumVal " ENUM_VAL_FMT " to string.\n",enumVal);
    return NULL;
}
#endif

#ifndef enumToStringSafe
const char* enumToStringSafe(size_t enumVal, const EnumLookupTable_t* lookupTable, size_t lookupTableLength) {
    const char* str = enumToString(enumVal, lookupTable, lookupTableLength);
    if (!str) return UNKNOWN_STR;
    return str;
}
#endif

#ifndef stringToEnum
size_t stringToEnum(const char* str, const EnumLookupTable_t* lookupTable, size_t lookupTableLength, size_t notFoundValue) {
    size_t idx;
    size_t len;
    // Remove leading whitespace
    for (idx = 0; isspace(*str); ++idx, ++str);
    // Remove trailing whitespace by liming compare length
    for (len = strlen(str); len && isspace(str[len-1]); --len);
    // Try to match enum
    for (idx = 0; len && (idx < lookupTableLength); idx++) {
        // Comparison length is at most the longer of the lookup table
        // entry and the input string (less whitespace) length.
        size_t compLen = strlen(lookupTable[idx].str);
        if (compLen < len) compLen = len;
        // Compare string with lookup
        if (!strncmp(str, lookupTable[idx].str, compLen)) {
#ifndef DISABLE_ENUM_LOOKUP_MATCH_LOG
            EnumDbgPrintEx("Matched string %s to enumVal " ENUM_VAL_FMT "\n", str, lookupTable[idx].enumVal);
#endif
            return lookupTable[idx].enumVal;
        }
    }
    EnumDbgPrintEx("Could not match string %s to enumVal. Using notFoundValue of " ENUM_VAL_FMT "\n", str, notFoundValue);
    return notFoundValue;
}
#endif
