/*
 * This program shows how to create a function pointer
 */
#include "HPS_Watchdog/HPS_Watchdog.h"
//Define a new data type for a function which takes two int parameters
//and returns an int.
typedef int (*TaskFunction)(int, int);
//Create a couple of functions which match our function data type
int aFunction(int a, int b){
    int c = a + b;
    return c;
}
int anotherFunc(int a, int b) { 
    int c = a * b;
    return c;
}
//Main Function
int main(void) {
    //Create a pointer to aFunction
    TaskFunction aPointer = &aFunction; //Try changing to &anotherFunc
    //Create some inputs
    int a = 4;
    int b = 6;
    //Try calling the function
    int c = aPointer(a, b);
    //Do nothing.
    while(1) {
        ResetWDT();
    }
}
