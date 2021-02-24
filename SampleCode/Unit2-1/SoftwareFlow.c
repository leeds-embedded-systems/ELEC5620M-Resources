while (1) { //Main Run Loop - runs forever.
    while (event1Occurred() == false);  //Wait for first event to occur
    do_something();                     //Do something
 
    while (event2Occurred() == false);  //Wait for second event to occur    
    do_something_else();                //Do something else
   
   //Were stuck in the loops waiting for events, so didn't do other things
}

while (1) { //Main Run Loop - runs forever.
   if (event1Occurred() == true){  //If event1 occurred
       do_something();             //Do something
   }
   if (event2Occurred() == true){  //If event2 occurred
       do_something_else();        //Do something else
   }
   //Otherwise we sit in run loop so can do other things.
}
