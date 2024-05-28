/*
 * Event Handler Driver
 * --------------------------
 * 
 * Uses a free-running 32-bit generic timer to provide a
 * multi-event scheduler with the option of calling back
 * a function when an event needs to be handled.
 * 
 * For event timing, a more complex functionality is provided
 * to allow multiple events to be run on the same timer, with
 * each event allowing free-run/one-shot with their own timeout
 * periods. This is more involved as it requires the help of a
 * polling API for events to check if they have occurred.
 * 
 * Event Timing
 * ------------
 * There are two types of event timing, Registered and Manual.
 * 
 * For Registered events, use Event_create to register a new event
 * which will be automatically checked by the Event_process
 * polling function. Registered events can have a callback
 * function associated with them which is called whenever the
 * event occurs (with the option to requeue or stop based on
 * return value).
 * 
 * For Manual events, use Event_init to initialse an event struct
 * to work with given timer. Manual events are not tracked by the
 * polling function and have no callback. Instead you should use
 * Event_checkState to see if it has reached the timeout period.
 * Manual events are requeued only if the call to Event_checkState
 * requests it.
 * 
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------------
 * 09/03/2024 | Split out event handling from HPS Timer
 *
 */

#ifndef EVENT_H_
#define EVENT_H_

#include "Util/macros.h"
#include "Util/driver_ctx.h"
#include "Util/driver_timer.h"

#include <stdbool.h>

#include "Util/error.h"

typedef enum {
    EVENT_TYPE_MANUAL =   -1, // Event is handled manually by Event_checkState.
    EVENT_TYPE_DISABLED =  0, // Event disabled
    EVENT_TYPE_ONESHOT,       // Event triggers once then stops. HPS timers are stopped by getInterruptFlag().
    EVENT_TYPE_REPEAT         // Event keeps triggering
} EventType;

enum {
    EVENT_STATE_ERROR =   ERR_UNKNOWN, // Event error occurred (e.g. timer API failed). Any HpsErr_t value is possible.
    EVENT_STATE_INVALID = 0,           // Event invalid, do not use (was likely destroyed)
    EVENT_STATE_DISABLED,              // Event disabled
    EVENT_STATE_PENDING,               // Event is waiting for timeout to be reached
    EVENT_STATE_OCCURRED               // Event occurred since last check
};

#define EVENT_STATE_ISDISABLED(x) ((x) <= EVENT_STATE_DISABLED)
#define EVENT_STATE_ISINVALID(x)  ((x) == EVENT_STATE_INVALID )
#define EVENT_STATE_SUCCESS(x)    ((x) >  EVENT_STATE_INVALID )

typedef enum {
    EVENT_CNTRL_CHECK,       // Check current state only
    EVENT_CNTRL_CANCEL,      // Cancel event if running. Will set state to DISABLED
    EVENT_CNTRL_ENQUEUE,     // Start the event only if currently in DISABLED state
    EVENT_CNTRL_RESTART      // Restart the event from the current time, even if running.
} EventControl;

#define EVENT_INTERVAL_UNCHANGED  0

// Event timing handler function
//  - Called when a registered event hits the timeout
//  - Return ERR_SUCCESS to stop handling the event
//  - Return ERR_AGAIN to continue handling the event
typedef struct _Event_t Event_t, *PEvent_t;
typedef HpsErr_t (*EventFunc_t)(PEvent_t event, void* param);

// Event Handling Context
typedef struct {
    //Header
    DrvCtx_t header;
    //Body
    PTimerCtx_t  timer;  // Timer used by this event manager.
    PEvent_t*    events; // "Registered" type events
    unsigned int count;
} EventMgrCtx_t, *PEventMgrCtx_t;

// Event structure
typedef struct _Event_t{
    HpsErr_t    state;        // Current state of the event (e.g. whether occurred)
    EventType      type;         // Whether the event is enabled and repeats
    unsigned int   interval;     // Interval between events in clock cycles
    unsigned int   lastTime;     // Last counter value that the event was handled
    PTimerCtx_t    timerCtx;     // Timer context.
    // Context entries used for registered events only:
    EventFunc_t    handler;      // Optional handler to call when event occurs
    void*          param;        // Optional parameter for handler
    PEventMgrCtx_t evtMgrCtx;    // Event Manager context
} Event_t, *PEvent_t;

// Event polling function
//  - Must call this function repeatedly in the main loop to keep checking
//    if any event has occurred
HpsErr_t Event_process(PEventMgrCtx_t ctx);

// Create a registered event
//  - Timer must be configured to TIMER_MODE_EVENT
//  - interval is the number of timer clock cycles before the event occurs. Must be >0.
//  - mode sets the initial state of the event (whether disabled, enabled once, or enabled repeating)
//  - Multiple events can be registered
//  - Will return event context to *pEvtCtx
HpsErr_t Event_create(PEventMgrCtx_t ctx, EventType type, unsigned int interval, EventFunc_t handler, void* param, PEvent_t* pEvtCtx);

// Initialise a manual event
//  - Will error if event is already initialised.
//  - If enqueue is true, will automatically start event timeout. Otherwise event will start disabled.
//  - To avoid strange behaviour, call Event_destroy(evt) before init.
HpsErr_t Event_init(PEvent_t evt, PTimerCtx_t timer, unsigned int interval, bool enqueue);

// Destroy an event
//  - Cancels the event by zeroing out its structure.
//  - Do NOT destroy an event from within its own handler function!
void Event_destroy(PEvent_t evt);

// Check if an event context is valid
//  - returns true if the event context is valid
bool Event_validate(PEvent_t evt);

// Check or control an event
//  - Pass in a event context returned from Event_create or Event_init.
//  - Returns the state of the event before any op was performed
//  - Performs control operation on event if not EVENT_CNTRL_CHECK.
//  - If restarting the event, and interval != EVENT_INTERVAL_UNCHANGED (0),
//    the interval of the event will be updated to match.
HpsErr_t Event_state(PEvent_t evt, EventControl op, unsigned int interval);

// Change mode for a timer event
//  - Pass in a timer event context returned from createEvent.
//  - Setting an interval of EVENT_INTERVAL_UNCHANGED (0) means keep the
//    previously set interval.
HpsErr_t Event_setMode(PEvent_t evt, EventType type, unsigned int interval);

// Event just occurred
//  - Returns true if a valid event occurred since the last check.
static inline bool Event_occurred(PEvent_t evt) {
    return (Event_state(evt, EVENT_CNTRL_CHECK, EVENT_INTERVAL_UNCHANGED) == EVENT_STATE_OCCURRED);
}

#endif
