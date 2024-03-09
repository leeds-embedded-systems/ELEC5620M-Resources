/*
 * General Use Macros
 * ------------------
 *
 * Provides some helpful macros.
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 28/12/2023 | Creation of header
 *
 */

#ifndef MACROS_H_
#define MACROS_H_

//Stringification Macro
#define _STRINGIFY(a) #a
#define STRINGIFY(a) _STRINGIFY(a)

//Concatenation Macro
#define _CONCAT(a,b) a##b
#define CONCAT(a,b) _CONCAT(a,b)

//For enums this will make them the smallest integer type which can fit the maximum value.
//For structs/unions it will ensure they are packed to the smallest size if applied.
#define PACKED __attribute__ ((packed))

//Whether field is aligned to certain boundary
#define ALIGNED(x) __attribute__ ((aligned(x)))

//Define EXTERN_C keyword if not already.
#ifndef EXTERN_C
#ifdef __cplusplus
#define EXTERN_C extern "C"
#else
#define EXTERN_C
#endif
#endif

//Fallthrough attribute for case statements.
#if ((defined(__GNUC__) && __GNUC__ >= 7) || (defined(__clang__) && __clang_major__ >= 12)) && !defined( __CDT_PARSER__)
 #define FALLTHROUGH __attribute__ ((fallthrough))
#else
 #define FALLTHROUGH ((void)0)
#endif

//Size of fixed array
#define ARRAYSIZE(arr) (sizeof((arr))/sizeof(*(arr)))
#define ARRAYWITHSIZE(arr) arr, ARRAYSIZE(arr)

//Integer ceiling Division
#ifndef CEIL_DIV
#define CEIL_DIV(a,b) (((a)+(b)-1)/(b))
#endif

//Integer rounding Division
#ifndef ROUND_DIV
#define ROUND_DIV(a,b) (((a)+(b)/2)/(b))
#endif

//Integer flooring Division
#ifndef FLOOR_DIV
#define FLOOR_DIV(a,b) ((a) / (b))
#endif

//Min and Max macros if allowed
#ifndef NOMINMAX

#ifndef min
#define min(a,b) ((a) > (b) ? (b) : (a))
#endif

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#endif

//Constants
// Pi
#ifndef M_PI
#define M_PI     3.14159265358979323846
#endif
// 2*Pi
#define M_2PI    (2*M_PI)
// e
#ifndef M_E
#define M_E      2.71828182845904523536
#endif

#endif /* MACROS_H_ */
