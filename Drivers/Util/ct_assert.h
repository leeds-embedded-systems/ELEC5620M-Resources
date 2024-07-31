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
 * -----------+----------------------------------
 * 28/12/2023 | Add ct_assert_aligned
 * 08/10/2014 | Creation of header
 *
 */

#ifndef CT_ASSERT_H_
#define CT_ASSERT_H_

//Assertion Checker.
//  - ct_assert(dataType, expectedSize) will at compile time issue an error if the data type is not the expected size.

#define ASSERT_CONCAT_(a, b) a##b
#define ASSERT_CONCAT(a, b) ASSERT_CONCAT_(a, b)
#define ct_assert(a,e) enum { ASSERT_CONCAT(assert_sizeof_, a) = 1/((size_t)!!(sizeof(a) == e)) }
#define ct_assert_define(a,op) enum { assert_define_##a = 1/((size_t)!!(a op)) }
#define ct_assert_aligned(a,v,s,e) enum { ASSERT_CONCAT(assert_sizeof_, a) = 1/((size_t)!!((v & (s-1)) == e)) }

#endif /* CT_ASSERT_H_ */
