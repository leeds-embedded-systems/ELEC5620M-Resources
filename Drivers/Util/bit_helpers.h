/*
 * Bit Access Helpers
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
 * 03/02/2024 | Add countBits implementation
 * 28/12/2023 | Creation of header
 *
 */

#ifndef BIT_HELPERS_H_
#define BIT_HELPERS_H_

#include <stdbool.h>

//Ensure we have __rbit and __clz used by some bit routines
#ifdef __arm__
    //ARM has both of these already defined in its instruction set
    #include <arm_acle.h>

    #define __popcount(x) __popcount_failback(x)

#elif defined(__NIOS2__)
    //Nios has building CLZ, and optional custom instruction for RBIT.
    #define __clz(x) __builtin_clz(x)

    #include <system.h>
    #ifdef ALT_CI_NIOS_BITSWAP
        #define __rbit(x) ALT_CI_NIOS_BITSWAP(x)
    #else
        //If we don't have the custom instruction, use fail back method
        #define __rbit(x) __rbit_failback(x)
    #endif

    #define __popcount(x) __builtin_popcount(x)

#else
    //Otherwise use fail-back methods
    #warning "Please re-define __clz and __rbit for your system type if accelerators exist."
    #define __clz(x)  __clz_failback(x)
    #define __rbit(x) __rbit_failback(x)
    #define __popcount(x) __popcount_failback(x)

#endif

// Fail-back implementation of popcount if not available.
// http://graphics.stanford.edu/~seander/bithacks.html
static inline unsigned int __popcount_failback(unsigned int x) {
    x = x - ((x >> 1) & 0x55555555U);
    x = (x & 0x33333333U) + ((x >> 2) & 0x33333333U);
    return ((x + (x >> 4) & 0xF0F0F0FU) * 0x1010101U) >> 24; // count
}

// Fail-back implementation of RBIT if not available.
// https://stackoverflow.com/a/9144870/1557472
static inline unsigned int __rbit_failback(unsigned int x) {
    x = ((x >> 1) & 0x55555555u) | ((x & 0x55555555u) << 1);
    x = ((x >> 2) & 0x33333333u) | ((x & 0x33333333u) << 2);
    x = ((x >> 4) & 0x0f0f0f0fu) | ((x & 0x0f0f0f0fu) << 4);
    x = ((x >> 8) & 0x00ff00ffu) | ((x & 0x00ff00ffu) << 8);
    x = ((x >> 16) & 0xffffu) | ((x & 0xffffu) << 16);
    return x;
}

// Fail-back implementation of CLZ if not available.
// https://stackoverflow.com/a/23857066/1557472
static inline unsigned int __clz_failback(unsigned int x) {
    unsigned int n = 32;
    unsigned int y;
    y = x >>16; if (y != 0) { n = n -16; x = y; }
    y = x >> 8; if (y != 0) { n = n - 8; x = y; }
    y = x >> 4; if (y != 0) { n = n - 4; x = y; }
    y = x >> 2; if (y != 0) { n = n - 2; x = y; }
    y = x >> 1; if (y != 0) return n - 2;
    return n - x;
}

//Bit value
#ifndef _BV
#define _BV(a) (1 << (a))
#endif

//Masked bit value helpers
// - Mask and shift a value to the correct position
#define MaskInsert(       val, mask, ofs) (((val) & (mask)) << (ofs))
// - Shift and mask a field from a register value
#define MaskExtract(      val, mask, ofs) (((val) >> (ofs)) & (mask))
// - Shift and mask a signed field from a register value
#define MaskExtractSigned(val, mask, ofs) ((MaskExtract(val, mask, ofs) ^ (((mask) + 1)/2)) - (((mask) + 1)/2))
// - Mask a register value to perform logical checks
#define MaskCheck(        val, mask, ofs) ((val) & ((mask) << (ofs)))
// - Create a bitmask for direct AND/OR with a register value.
#define MaskCreate(            mask, ofs) ((mask) << (ofs))
// - Read-Modify-Write value with new masked value.
#define MaskModify(  reg, val, mask, ofs) (((reg) & ~MaskCreate(mask, ofs)) | MaskInsert(val, mask, ofs))

// Given a base address, calculate the maximum power of two span the address can access as a bitmask
#define MaxAddressSpanOfBaseMask(base) (((base) & -(base)) - 1)

// Reverse the bytes in a 32-bit word
static inline __attribute__((always_inline)) unsigned int reverseInt(unsigned int data) {
    // compiler should optimise this to __REV instruction
    data = ((data & 0xFF000000) >> 24) | ((data & 0x00FF0000) >> 8) | ((data & 0x0000FF00) << 8) | ((data & 0x000000FF) << 24);
    return data;
}

// Reverse the bytes in a 16-bit word
static inline __attribute__((always_inline)) unsigned short reverseShort(unsigned short data) {
    // compiler should optimise this to __REV16 instruction
    data = ((data & 0xFF00) >> 8) | ((data & 0x00FF) << 8);
    return data;
}

// Check if address alignment is correct
static inline bool pointerIsAligned(const void* ptr, unsigned int size) {
    return !(((unsigned int)ptr) & (size - 1));
}

// Align an address or length to a size boundary
static inline void* alignPointer(const void* ptr, unsigned int size, bool toNext) {
    unsigned int val = (unsigned int)ptr;
    if (toNext) val = val + (size - 1);
    return (void*)(val & ~(size - 1));
}

// Find most significant set bit (effectively performs floor(log2(x)))
// - Returns the index of the highest set bit
// - Returns -1 if no bits set
static inline signed int findHighestBit(unsigned int x) {
    return 31-__clz(x);
}

// Performs the hamming count on a 32bit integer (counts ones)
static inline unsigned int countOnes(unsigned int x) {
    return __popcount(x);
}

// Optimised function for finding least significant 0 in a bitmask.
static inline unsigned int findFirstZero(unsigned int x) {
    //First invert, as intrinsics are looking for 1's
    x = ~x;
    //Then flip bit order so that we can look for first MSB
    x = __rbit(x);
    //Then count how many leading zeros there are (indicates the position of the first 1)
    return __clz(x);
}

#if __BYTE_ORDER == __LITTLE_ENDIAN
# define cpu_to_le16(x)     (x)
# define cpu_to_le32(x)     (x)
# define le16_to_cpu(x)     (x)
# define le32_to_cpu(x)     (x)
# define cpu_to_be16(x)     reverseShort(x)
# define cpu_to_be32(x)     reverseInt(x)
# define be16_to_cpu(x)     reverseShort(x)
# define be32_to_cpu(x)     reverseInt(x)
#else
# define cpu_to_le16(x)     reverseShort(x)
# define cpu_to_le32(x)     reverseInt(x)
# define le16_to_cpu(x)     reverseShort(x)
# define le32_to_cpu(x)     reverseInt(x)
# define cpu_to_be16(x)     (x)
# define cpu_to_be32(x)     (x)
# define be16_to_cpu(x)     (x)
# define be32_to_cpu(x)     (x)
#endif
typedef unsigned long       ulong;

#endif /* BIT_HELPERS_H_ */

