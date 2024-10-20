/*
 * Compile Time Assert
 * -------------------
 *
 * Compile time assert statements used for checking
 * the alignment and size of data types.
 *
 * For example:
 *
 *    ct_assert(MyType, sizeof(uint32_t));
 *
 * Adding below a data typedef or anywhere in the code
 * will cause a compile error if the size of MyType is
 * not equal to the number of bytes specified.
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+------------------------------------
 * 05/10/2024 | Update comments and guard arguments
 * 28/12/2023 | Add ct_assert_aligned
 * 08/10/2014 | Creation of header
 *
 */

#ifndef CT_ASSERT_H_
#define CT_ASSERT_H_

// Internal macros
#define ASSERT_CONCAT_(a, b) a##b
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)

//-----------------------------------------------------------------------------   
// Assertion Checker.
// The macros produce a compile time error if the condition is not met:
//  - ct_assert(dataType, expectedSize)                 Type is not the expected size:          ct_assert(data_type, 4)               data type must be 4 bytes
//  - ct_assert_define(hashDefine, op)                  #define doesn't pass the operation:     ct_assert_define(MY_MACRO, <=6)       define must be less or equal 6
//  - ct_assert_define_range(hashDefine, opmin, opmax)  #define doesn't pass two operations:    ct_assert_define(MY_MACRO, >2, <=5)   define must be in range (2 5]
//  - ct_assert_aligned(name, val, 1<<width, rem)       Value masked to width equals remainder: ct_assert_aligned(NAME, 123, 1<<4, 0) value must be 4-bit aligned
//----------------------------------------------------------------------------- 
#define ct_assert(a,e) enum { ASSERT_CONCAT(assert_sizeof_, a) = 1/((size_t)!!(sizeof(a) == (e))) }
#define ct_assert_define(a,op) enum { assert_define_##a = 1/((size_t)!!((a) op)) }
#define ct_assert_define_range(a,opmin,opmax) enum { assert_define_min_##a = 1/((size_t)!!((a) opmin)), assert_define_max_##a = 1/((size_t)!!((a) opmax))}
#define ct_assert_aligned(a,v,s,e) enum { ASSERT_CONCAT(assert_sizeof_, a) = 1/((size_t)!!(((v) & ((s)-1)) == (e))) }

#endif /* CT_ASSERT_H_ */
