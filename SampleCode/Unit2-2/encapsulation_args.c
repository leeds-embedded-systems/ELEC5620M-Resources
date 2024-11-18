// Configure a timer for a given time period (in ms)
void timer_configure(unsigned int* timerPtr, unsigned int prescalar, 
                     unsigned int frequency, unsigned int timePeriod) {
    unsigned int loadVal = frequency * timePeriod / (prescalar + 1) / 1000;
    //...
}
// Display the value on segments
void sevenseg_display(unsigned int* hexPtr, unsigned int value) {
    //...
}
// Start the timer and display how long is left
void startTimeout(unsigned int* hexPtr, unsigned int* timerPtr, 
                  unsigned int prescalar, unsigned int frequency, 
                  unsigned int timePeriod) {
    timer_configure(timerPtr, prescalar, frequency, timePeriod);
    sevenseg_display(hexPtr, timePeriod);
}
// Main program
int main (void) {
    //...
    unsigned int* hexPtr   = ...;
    unsigned int* timerPtr = ...;
    unsigned int prescalar = ...;
    unsigned int frequency = ...;
    //...
        unsigned int timePeriod = 10;
        startTimeout(hexPtr, timerPtr, prescalar, frequency, timePeriod);
    //...
}


