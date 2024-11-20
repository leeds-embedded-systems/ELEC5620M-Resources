// Example function returning value via pointer
//  - returns error code directly
//  - returns another value by by writing to retValue
HpsErr_t apiReturningViaPointer(...,  unsigned int* retValue) {
    // Validate that return address is valid
    if (!returnVal) return ERR_NULLPTR; // Can't write to a null pointer!
    // The value we want to return
    unsigned int myValue = ...;
    // Return the value by writing to memory address
    *retValue = myValue;
    // We are free then to return an error code directly
    return ERR_SUCCESS;
}

// Example of calling the function
unsigned int result;
HpsErr_t errorCode;
// Call the function
errorCode = apiReturningViaPointer(..., &result);
// errorCode contains error value
// result now contains the data value returned.
