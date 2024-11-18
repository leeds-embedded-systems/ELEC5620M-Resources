// Struct as variable
struct {
    unsigned int aVal;
    char* anotherVal;
    struct {
        unsigned int innerField;
    } nestedStruct;
} myVar;

// Define a new struct data type to store multiple values
typedef struct {
    // Different values the struct encapsulates
    unsigned int* base;
    unsigned int  prescalar;
    unsigned int  frequency;
    bool          isRunning;
} timer_info; // Name of the new data type

// ...

// Creating a variable for a struct data type is the same as any other variable.
//  - We can create one which is unininitialised (all values unknown)
timer_info timer;
//  - We could initialise all fields to 0:
timer_info timer = {0};
//  - Or we could initialise fields to specific values (any fields we
//    dont specific will be initialsed to 0).
timer_info timer = {
    .base = 0x1234,
    .prescalar = 0,
    .isRunning = false
};

// We can then access struct fields
//  - We simply put a '.' then the field name to access a specific field
myVar.aVal = 1;
myVar.nestedStruct.innerField = 2;
timer.frequency = 125000000;

// Pointers to struct (makes the typedef verion more useful)
//  - Just like any data type, we can make a pointer to a struct
//  - This can be passed efficiently to a function
timer_info* timerPtr = &timer;

// Accessing struct pointer fields.
//  - If we have a pointer to a struct, we access it slightly differently
//  - Use '->' to access a field within a struct pointer
unsigned int curFreq = timerPtr->frequency / (timerPtr->prescalar + 1);
