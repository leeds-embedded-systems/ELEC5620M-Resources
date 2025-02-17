/*
 * Example header with include guards
 */
//Note MYHEADER_H_ is typically the filename in all-caps
//replacing the . with _, and placing an _ at the end.
#ifndef MYHEADER_H_
#define MYHEADER_H_
#define SOME_CLEVER_MACRO()  (Do stuff)
typedef unsigned int myDataType;
#endif /*MYHEADER_H_*/

/*
 * C code including it
 */
#include "MyHeader.h"
#include "MyHeader.h"
// Do stuff

/*
 * Result seen by compiler:
 */
// Initially, MYHEADER_H_ is not defined
// so this contents of the first #ifndef is true and so the
// code within is compiled.
#ifndef MYHEADER_H_
#define MYHEADER_H_
#define SOME_CLEVER_MACRO()  (Do stuff)
typedef unsigned int myDataType;
#endif /*MYHEADER_H_*/
// Because MYHEADER_H_ is now already defined by the previons
// copy, the second (and future) #ifndef is therefore false
// and code within thus excluded from the compile
#ifndef MYHEADER_H_
#define MYHEADER_H_
#define SOME_CLEVER_MACRO()  (Do stuff)
typedef unsigned int myDataType;
#endif /*MYHEADER_H_*/
// Do stuff
