/*
 * Template
 * ------------------------
 *
 * Template Driver
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 24/01/2024 | Creation of driver.
 *
 */

#ifndef TEMPLATE_H_
#define TEMPLATE_H_

#include "Util/driver_ctx.h"

typedef struct {
    //Header
    DrvCtx_t header;
    //Body
    volatile unsigned int* base;
} TemplateCtx_t;

// Initialise the Template Driver
//  - base is a pointer to the template thingy
//  - Returns Util/error Code
//  - Returns context pointer to *ctx
HpsErr_t Template_initialise(void* base, TemplateCtx_t** pCtx);

// Check if driver initialised
//  - Returns true if driver previously initialised
bool Template_isInitialised(TemplateCtx_t* ctx);

// Template API
//  - Does stuff.
HpsErr_t Template_api(TemplateCtx_t* ctx);


#endif /* TEMPLATE_H_ */

