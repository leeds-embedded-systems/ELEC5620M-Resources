//Always check the FIFO Space before writing or reading left/right channel pointers
if (space > 0) {

    /******* Time Code Execution Between Here *******/
    //If there is space in the write FIFO for both channels:
    //Increment the phase
    phase = phase + inc;
    //Ensure phase is wrapped to range 0 to 2Pi (range of sin function)
    while (phase >= PI2) {
        phase = phase - PI2;
    }
    // Calculate next sample of the output tone.
    audio_sample = (signed int)( ampl * sin( phase ) );
    // Output tone to left and right channels.
    WM8731_writeSample(audioCtx, audio_sample, audio_sample);
    /******* And Here *******/
}