// Global variables to share between functions
unsigned int* hexPtr   = ...;
unsigned int* timerPtr = ...;
unsigned int prescalar = ...;
unsigned int frequency = ...;
// Configure a timer for a given time period (in ms)
void timer_configure(unsigned int timePeriod) {
    unsigned int loadVal = frequency * timePeriod / (prescalar + 1) / 1000;
    //...
}
// Display the value on segments
void sevenseg_display(unsigned int value) {
    //...
}
// Main program
int main (void) {
    //...
    prescalar = 0;
    frequency = 125000000;
    //...
        unsigned int timePeriod = 10;
        timer_configure(timePeriod);
        sevenseg_display(timePeriod);
    //...
}
