// Lets create a variable and a pointer
int aVariable = 12;    //Create an integer variable (int), and set it's value to 12
int* aPointer;         //Create a pointer variable (int*) - note the * in the type
int anotherVar;        //Create another variable.
// The memory map could now looks like the following:
//    Address  | Variable   | Value
//   ----------+------------+---------
//    0x010100 | aVariable  | 12
//    0x010104 | aPointer   | ?
//    0x010108 | anotherVar | ?


aPointer = &aVariable; //Get the address of aVariable (&), and store it in aPointer
// The memory now looks like the following:
//    Address  | Variable   | Value
//   ----------+------------+---------
//    0x010100 | aVariable  | 12
//    0x010104 | aPointer   | 0x010100
// Notice how aVariable is 12, while aPointer has a value of 0x010100 (&aVariable)

//Lets try accessing the pointer
return aPointer;  //Return the value of the pointer - this will return 0x010100
return *aPointer; //Dereference (*) to access value stored at the address - return 12


// Declare an array of 8 values, and set the elements to an initial value.
char anArray[8] = {0,1,1,2,3,5,8,13};
// The memory might now look like the following:
//    Address  | Variable   | Value
//   ----------+------------+---------
//    0x030800 | anArray[0] | 0
//    0x030801 | anArray[1] | 1
//    0x030802 | anArray[2] | 1
//    0x030803 | anArray[3] | 2
//    0x030804 | anArray[4] | 3
//    0x030805 | anArray[5] | 5
//    0x030806 | anArray[6] | 8
//    0x030807 | anArray[7] | 13
// Notice the values of anArray are stored in sequential addresses.
// Lets try accessing the array 
return anArray[1]; //Return the value of element 1 - this will return a value of 1
return anArray[6]; //Return the value of element 6 - this will return a value of 8


// Create a pointer, which is equal to anArray (which is itself a pointer).
char* aPointer = anArray; // This does _not_ copy the array, it copies the address.
// The memory might now look like the following:
//    Address  | Variable   | Value
//   ----------+------------+---------
//    0x030800 | ?          | 0
//    0x030801 | ?          | 1
//    0x030802 | ?          | 1
//    0x030803 | ?          | 2
//    0x030804 | ?          | 3
//    0x030805 | ?          | 5
//    0x030806 | ?          | 8
//    0x030807 | ?          | 13
//    0x030808 | aPointer   | 0x030800
// Notice the values of anArray are stored in sequential addresses, but that anArray 
// is actually just representing the address 0x030800.
// Lets try accessing the array 
return *(aPointer+1); //Add 1 to aPointer, then dereference - will return value of 1
return *(aPointer+6); //Add 6 to aPointer, then dereference - will return value of 8


// Declare an array of 2 values, and set the elements to an initial value.
int  anArray[2] = {123456,543210};
int* aPointer = anArray;
// The memory might now look like the following:
//    Address  | Variable   | Value
//   ----------+------------+---------
//    0x021000 | anArray[0] | 64    //123456 & 0x000000FF
//    0x021001 | anArray[0] | 226   //123456 & 0x0000FF00
//    0x021002 | anArray[0] | 1     //123456 & 0x00FF0000
//    0x021003 | anArray[0] | 0     //123456 & 0xFF000000
//    0x021004 | anArray[1] | 234   //543210 & 0x000000FF
//    0x021005 | anArray[1] | 73    //543210 & 0x0000FF00
//    0x021006 | anArray[1] | 8     //543210 & 0x00FF0000
//    0x021007 | anArray[1] | 0     //543210 & 0xFF000000
//    0x030808 | aPointer   | 0x021000
// Notice the values of anArray take 4 bytes each. The ARM processor we use is
// Little-Endian, so lowest byte is in lowest address.
// Lets try accessing the array 
return anArray[0]; //Return value of element 0 - this will return a value of 123456
return anArray[1]; //Return value of element 1 - this will return a value of 543210
return *(aPointer+1); //Add 1 to aPointer, then dereference - will return 543210


// Create a "pointer" which is set to the base address of the Red LEDs.
volatile unsigned int *LEDR_ptr = (unsigned int *) 0xFF200000;
// To access the LEDs, we deference the pointer just like in the previous examples.
*LEDR_ptr = 16; //Use the dereference operator to set the value of the LEDs to 16.


// Create a "pointer" which is set to the base address of the switches.
volatile unsigned int *SW_ptr = (unsigned int *) 0xFF200040;
//Create a local variable to store the value of the switches
unsigned int SW_value;
// To access the switch value, we deference the pointer as in the previous examples.
SW_value = *SW_ptr; //Use the dereference operator to read the value of the switches.


// To access the switch value in a robust way, we deference the pointer just like in
// the previous examples and bit mask all the valid input the value.
// Bit mask 0x3FF is equal to the 32 bit, zero padded, binary value 1111111111.
// The bitwise AND (\&) operation would clear any erronious bits in *SW_ptr.
SW_value = (*SW_ptr & 0x3FF); //Use the dereference operator to read switch value,
                              //bitmask and set local variable.


// To access an individual switch value we deference the pointer just like in
// the previous examples and bit mask the required bit.
// Bit mask 0x2 is equal to the 32 bit, zero padded, binary value 0...0010 and
// and when combined with the bitwise AND (\&) would isolate the second bit.
unsigned int SW2_value;
SW2_value = (*SW_ptr & 0x2); //Use the dereference operator to read switch value,
                             //bitmask and set local variable.


// Create a "pointer" which is set to the base address of the
// 7-segment HEX[3], HEX[2], HEX[1] & HEX[0] Displays.
// Can access each segment individually, either with pointer
// arithmetic, or array access (both options work):
volatile unsigned char *sevenseg_ptr = (unsigned char *)0xFF200020;
*(sevenseg_ptr+1) = 0x7F; // Set HEX1 to show the text "8"
sevenseg_ptr[3] = 0x3;    // Set HEX3 to show the text "1" 
