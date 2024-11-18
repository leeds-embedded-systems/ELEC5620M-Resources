//Struct to encapsulate timer info
typedef struct {
    unsigned int* base;
    unsigned int  prescalar;
    unsigned int  frequency;    
} timer_info;
// Configure a timer for a given time period (in ms)
void timer_configure(timer_info* timerPtr, unsigned int timePeriod) {
    unsigned int loadVal = timerPtr->frequency * timePeriod / 
                           (timerPtr->prescalar + 1) / 1000;
    //...
}
// Display the value on segments
void sevenseg_display(unsigned int* hexPtr, unsigned int value) {
    //...
}
// Start the timer and display how long is left
void startTimeout(unsigned int* hexPtr, timer_info* timerPtr,
                  unsigned int timePeriod) {
    timer_configure(timerPtr, timePeriod);
    sevenseg_display(hexPtr, timePeriod);
}
// Main program
int main (void) {
    //...
    unsigned int* hexPtr = ...;
    timer_info timer = {0};
    timer.base =      ...;
    timer.prescalar = ...;
    timer.frequency = ...;
    //...
        unsigned int timePeriod = 10;
        startTimeout(hexPtr, &timer, timePeriod);
    //...
}


