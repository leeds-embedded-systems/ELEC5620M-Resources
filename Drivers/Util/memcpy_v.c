/*
 * Volatile Respecting Memcpy
 * --------------------------
 *
 * Provides an instantiation of memcpy which does not
 * discard the volatile qualifier
 *
 * Based on sample from StackOverflow. Licensed under
 * CC BY-SA 4.0
 * https://stackoverflow.com/a/54965696/1557472
 *
 */

#include "memcpy_v.h"

// memcpy for Volatile Source/Destiation
volatile void *memcpy_v2v(volatile void *restrict dest,
            const volatile void *restrict src, size_t n
) {
    const volatile unsigned char *src_c = src;
    volatile unsigned char *dest_c      = dest;

    while (n > 0) {
        n--;
        dest_c[n] = src_c[n];
    }
    return  dest;
}

// memcpy for Volatile Source
volatile void *memcpy_v2(void *restrict dest,
            const volatile void *restrict src, size_t n
) {
    const volatile unsigned char *src_c = src;
    unsigned char *dest_c               = dest;

    while (n > 0) {
        n--;
        dest_c[n] = src_c[n];
    }
    return  dest;
}

// memcpy for Volatile Destination
volatile void *memcpy_2v(volatile void *restrict dest,
            const void *restrict src, size_t n
) {
    const unsigned char *src_c     = src;
    volatile unsigned char *dest_c = dest;

    while (n > 0) {
        n--;
        dest_c[n] = src_c[n];
    }
    return  dest;
}
