/*
 * Cyclone V HPS Interrupt Controller
 * ------------------------------------
 * Description:
 * Driver for enabling and using the General Interrupt
 * Controller (GIC). The driver includes code to create
 * a vector table, and register interrupts.
 *
 * The code makes use of function pointers to register
 * interrupt handlers for specific interrupt IDs.
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 12/03/2018 | Creation of driver
 *
 */

#ifndef HPS_IRQ_H_
#define HPS_IRQ_H_

#include <stdbool.h>
#include <stdlib.h>

#define HPS_IRQ_SUCCESS        0
#define HPS_IRQ_ERRORNOINIT   -1
#define HPS_IRQ_NOTFOUND      -2
#define HPS_IRQ_NONENOUGHMEM  -4


//Include a list of IRQ IDs that can be used while registering interrupts
#include "HPS_IRQ_IDs.h"

//Function Pointer Type for Interrupt Handlers
// - interruptID is the ID of the interrupt that called the handler.
// - When IRQ handler is called by interrupt, 'isInit' is false and 'initParams'
//   can be ignored.
// - If variables need to be shared with the handler, the user can call the irq
//   handler manually with 'isInit' set to true, and 'initParams' a pointer to
//   the variables that need to be shared.
typedef void (*isr_handler_func)(HPSIRQSource interruptID, bool isInit, void* initParams);
// Two examples of IRQ handlers are as follows:
/* ---Start Examples---
//Handler not needing parameters
void exampleIRQHandlerWithNoParams(HPSIRQSource interruptID, bool isInit, void* initParams) {
    if (!isInit) {
       //IRQ handler stuff...   
       
    }
}
//Handler needing parameters
void exampleIRQHandlerUsingParams(HPSIRQSource interruptID, bool isInit, void* initParams) {
    static someVarType* params = NULL;
    if (isInit) {
       //Initialise the params variable. 
        params = (someVarType*)initParams;
    } else if (params != NULL) {
       //IRQ handler stuff...   
       
    } //Optionally, else { //Do something if interrupted before initialised  } 
}
// ---End Examples--- */


//Initialise HPS IRQ Driver
// - userUnhandledIRQCallback is either a function pointer to an isr_handler to
//   be called in the event of an unhandled IRQ occurring. If this parameter is
//   passed as NULL (0x0), a default handler which causes crash by watchdog will
//   be used.
// - Returns HPS_IRQ_SUCCESS if successful.
signed int HPS_IRQ_initialise(isr_handler_func userUnhandledIRQCallback );

//Check if driver initialised
// - Returns true if driver previously initialised
bool HPS_IRQ_isInitialised(void);

//Register a new interrupt ID handler
// - interruptID is the number between 0 and 255 of the interrupt being configured
// - handlerFunction is a function pointer to the function that will be called when IRQ with ID occurs
// - if a handler already exists for the specified ID, it will be replaced by the new one.
// - the interrupt ID will be enabled in the GIC
// - returns HPS_IRQ_SUCCESS on success.
// - returns HPS_IRQ_NONENOUGHMEM if failed to reallocated handler array.
signed int HPS_IRQ_registerHandler(HPSIRQSource interruptID, isr_handler_func handlerFunction);

//Unregister interrupt ID handler
// - interruptID is the number between 0 and 255 of the interrupt being configured
// - the interrupt will be disabled also in the GIC
// - returns HPS_IRQ_SUCCESS on success.
// - returns HPS_IRQ_NOTFOUND if handler not found
signed int HPS_IRQ_unregisterHandler(HPSIRQSource interruptID);

#endif /* HPS_IRQ_H_ */
