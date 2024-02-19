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
 * 30/01/2024 | Adapt for embedded code from DLL
 * 30/01/2018 | Creation of utility
 *
 */

#if defined(_WIN32)
    #include "EnumLookup.h"
    #include "DebuggerOutput.h"
#elif defined( __linux__ )
    #include "EnumLookup.h"
    #include "DebuggerOutput.h"
    #include "StringSafe.h"
#elif defined(__arm__) || defined(__arm_on_nios__)
    #include "enum_lookup.h"
    #include "Util/verbosity.h"
    #ifndef DEBUG
        #define DLLDbgPrintEx(...) DbgPrintf(VERBOSE_EXTRAINFO,__VA_ARGS__)
    #endif
#else
    #include "EnumLookup.h"
#endif

#ifndef DLLDbgPrintEx
#define DLLDbgPrintEx(...)
#endif

#ifndef enumToString
const char* enumToString(size_t enumVal, const EnumLookupTable_t* lookupTable, size_t lookupTableLength) {
    for (size_t idx = 0; idx < lookupTableLength; idx++) {
        if (enumVal == lookupTable[idx].enumVal) {
            DLLDbgPrintEx("Matched enumVal %d to string %s\n", enumVal, lookupTable[idx].str);
            return lookupTable[idx].str;
        }
    }
    DLLDbgPrintEx("Could not match enumVal %d to string.\n",enumVal);
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
    for (size_t idx = 0; idx < lookupTableLength; idx++) {
        if (!strcmp(str, lookupTable[idx].str)) {
            DLLDbgPrintEx("Matched string %s to enumVal %d\n", str, lookupTable[idx].enumVal);
            return lookupTable[idx].enumVal;
        }
    }
    DLLDbgPrintEx("Could not match string %s to enumVal. Using notFoundValue of %d\n", str, notFoundValue);
    return notFoundValue;
}
#endif
