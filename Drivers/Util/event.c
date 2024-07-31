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


#include "event.h"


/*
 * Internal Functions
 */

// Check if the task has reached its interval
//  - If so, update its state and call any handler
//  - Event must be validated before calling this.
static void _Event_checkOccured(PEvent_t evt, unsigned int curTime) {
    // Skip any events in disabled state
    if (EVENT_STATE_ISDISABLED(evt->state)) return;
    // Check if time has elapsed
    if ((evt->lastTime - curTime) >= evt->interval) {
        // Mark as occurred
        evt->state = EVENT_STATE_OCCURRED;
        // Handle registered events
        if (evt->type != EVENT_TYPE_MANUAL) {
            // Call handler if there is one
            if (evt->handler) {
                // Call the handler and check if we should repeat
                HpsErr_t evtStatus = evt->handler(evt, evt->param);
                if (evtStatus == ERR_AGAIN) {
                    // Handler requested task repeat. Set to pending an update type
                    evt->state = EVENT_STATE_PENDING;
                    evt->type = EVENT_TYPE_REPEAT;
                } else if (ERR_IS_ERROR(evtStatus)) {
                    // Fatal error in handler
                    evt->state = evtStatus;
                } else {
                    // No more repeats to do. Disable the task
                    evt->state = EVENT_STATE_DISABLED;
                    evt->type = EVENT_TYPE_DISABLED;
                }
            } else if (evt->type == EVENT_TYPE_ONESHOT) {
                // Otherwise for one-shot events, disable
                evt->type = EVENT_TYPE_DISABLED;
            }
            // To avoid accumulation errors, we make sure to mark the last time
            // the task was run as when we expected to run it. Counter is going
            // down, so subtract the interval from the last time.
            evt->lastTime = evt->lastTime - evt->interval;
        }
    }
}

static void _EventMgr_cleanup(PEventMgrCtx_t ctx) {
    //Clean up any event handler contexts
    if (ctx->events) {
        //Loop through the table deleting any valid entries
        for (unsigned int tableIdx = 0; tableIdx < ctx->count; tableIdx++) {
            PEvent_t evt = ctx->events[tableIdx];
            if (evt) {
                evt->state = EVENT_STATE_INVALID;
                free(evt);
            }
        }
        //And then delete the table.
        free(ctx->events);
        ctx->count = 0;
        ctx->events = NULL;
    }
}

/*
 * User Facing APIs
 */


// Initialise Event Manager (Optional)
//  - An event manager instance is required only for registered events.
//    Can use Manual events without a manager.
//  - timer is a pointer to the timer to use.
//    - Must be configured in event mode.
//  - Returns Util/error Code
//  - Returns context pointer to *ctx
HpsErr_t EventMgr_initialise(TimerCtx_t* timer, PEventMgrCtx_t* pCtx) {
    //Ensure timer is valid
    TimerMode mode;
    HpsErr_t status = Timer_getMode(timer, &mode);
    if (ERR_IS_ERROR(status)) return status;
    if (mode != TIMER_MODE_EVENT) return ERR_INUSE;
    //Allocate the driver context, validating return value.
    status = DriverContextAllocateWithCleanup(pCtx, &_EventMgr_cleanup);
    if (ERR_IS_ERROR(status)) return status;
    //Save context values
    PEventMgrCtx_t ctx = *pCtx;
    ctx->timer = timer;
    //Now initialised
    DriverContextSetInit(ctx);
    return ERR_SUCCESS;
}

//Check if driver initialised
bool EventMgr_isInitialised(PEventMgrCtx_t ctx) {
    return DriverContextCheckInit(ctx);
}

// Registered event polling function
//  - If using registered events (CreateEvent), this is the processing
//    function which handles callbacks for those events.
//  - Must call this function repeatedly in the main loop to keep checking
//    if any event has occurred
HpsErr_t Event_process(PEventMgrCtx_t ctx) {
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //Ensure timer is in correct mode
    TimerMode mode;
    status = Timer_getMode(ctx->timer, &mode);
    if (ERR_IS_ERROR(status)) return status;
    if (mode != TIMER_MODE_EVENT) return ERR_INUSE;
    //Check the time
    unsigned int curTime;
    status = Timer_getTime(ctx->timer, &curTime);
    if (ERR_IS_ERROR(status)) return status;
    // Check if it is time to run each task
    for (unsigned int taskID = 0; (taskID < ctx->count) && ctx->events; taskID++) {
        // Skip any invalid or disabled tasks
        PEvent_t evt = ctx->events[taskID];
        if (!Event_validate(evt) || (evt->type == EVENT_TYPE_DISABLED)) {
            continue;
        }
        // Check if the task has reached its interval
        _Event_checkOccured(evt, curTime);
        // If a fatal error in the event handler occurred, give up.
        if (ERR_IS_ERROR(evt->state)) return evt->state;
    }
    //Keep the timer interrupt flag clear
    Timer_checkOverflow(ctx->timer, true);
    return ERR_SUCCESS;
}

// Create a registered event
//  - Timer must be configured to TIMER_MODE_EVENT
//  - interval is the number of timer clock cycles before the event occurs. Must be >0.
//  - mode sets the initial state of the event (whether disabled, enabled once, or enabled repeating)
//  - Multiple events can be registered
//  - Will return event context to *pEvtCtx
HpsErr_t Event_create(PEventMgrCtx_t ctx, EventType type, unsigned int interval, EventFunc_t handler, void* param, PEvent_t* pEvtCtx) {
    if (!pEvtCtx) return ERR_NULLPTR;
    *pEvtCtx = NULL;
    //Ensure context valid and initialised
    HpsErr_t status = DriverContextValidate(ctx);
    if (ERR_IS_ERROR(status)) return status;
    //If there is no handler table, ensure length is zero in case realloc fails
    if (!ctx->events) {
        ctx->count = 0;
    }
    //See if there is an invalid handler we can reuse
    PEvent_t evt = NULL;
    unsigned int tableIdx = 0;
    for (; tableIdx < ctx->count; tableIdx++) {
        PEvent_t checkCtx = ctx->events[tableIdx];
        if (!checkCtx) {
            //Empty table entry. Will reuse table index, but make a new entry
            break;
        } else if ((checkCtx->state == EVENT_STATE_INVALID) || !checkCtx->evtMgrCtx || !checkCtx->timerCtx) {
            //Invalid entry that belongs to nobody. We will reuse.
            evt = checkCtx;
            break;
        }
    }
    //If a free index was not found, grow the table
    if (tableIdx == ctx->count) {
        //Increase the size of the table by 1
        unsigned int count = ctx->count + 1;
        PEvent_t* newTable = (PEvent_t*)realloc(ctx->events, count * sizeof(*ctx->events));
        if (!newTable) {
            return ERR_ALLOCFAIL;
        }
        //Save the new table
        ctx->events = newTable;
        ctx->count = count;
    }
    //If we have no event context, allocate a new one and assign to table
    if (!evt) {
        evt = (PEvent_t)malloc(sizeof(*evt));
        ctx->events[tableIdx] = evt;
        if (!evt) return ERR_ALLOCFAIL;
    }
    //Initialise the (new) event
    evt->evtMgrCtx = ctx;
    evt->timerCtx = ctx->timer;
    evt->type = type;
    evt->interval = interval;
    evt->handler = handler;
    evt->param = param;
    evt->state = EVENT_STATE_DISABLED;
    evt->lastTime = 0;
    //And return the new event context
    *pEvtCtx = evt;
    return ERR_SUCCESS;
}

// Initialise a manual event
//  - Will error if event is already initialised.
//  - To avoid strange behaviour, call Event_destroy(evt) before init.
HpsErr_t Event_init(PEvent_t evt, PTimerCtx_t timer, unsigned int interval, bool enqueue) {
    // Ensure event pointer is valid and not already initialised
    if (!evt) return ERR_NULLPTR;
    if (!EVENT_STATE_ISINVALID(evt->state) || (evt->state != EVENT_TYPE_DISABLED)) return ERR_INUSE;
    // Initialise the manual event
    evt->timerCtx = timer;
    evt->type = EVENT_TYPE_MANUAL;
    evt->interval = interval;
    evt->handler = NULL;
    evt->param = NULL;
    evt->state = EVENT_STATE_DISABLED;
    evt->lastTime = 0;
    // If we are requesting automatic start, do it
    if (enqueue) {
        return Event_state(evt, EVENT_CNTRL_ENQUEUE, EVENT_INTERVAL_UNCHANGED);
    }
    return ERR_SUCCESS;
}

// Check if an event context is valid
//  - returns true if the event context is valid
bool Event_validate(PEvent_t evt) {
    return evt && !EVENT_STATE_ISINVALID(evt->state) && Timer_isInitialised(evt->timerCtx);
}

// Destroy an event
//  - Cancels the event by zeroing out its structure.
//  - Do NOT destroy an event from within its own handler function!
void Event_destroy(PEvent_t evt) {
    //Zero out the structure. This will mark it as invalid.
    memset(evt, 0, sizeof(*evt));
}

// Check or control an event
//  - Pass in a event context returned from Event_create or Event_init.
//  - Returns the state of the event before any op was performed
//  - Performs control operation on event if not EVENT_CNTRL_CHECK.
HpsErr_t Event_state(PEvent_t evt, EventControl op, unsigned int interval) {
    //Check if this is a valid event
    if (!Event_validate(evt)) return EVENT_STATE_INVALID;
    // Read the current state.
    HpsErr_t curState = evt->state;
    // Get the restart time
    unsigned int curTime;
    HpsErr_t status = Timer_getTime(evt->timerCtx, &curTime);
    if (ERR_IS_ERROR(status)) return EVENT_STATE_ERROR;
    // Check what operation we are performing
    switch (op) {
        case EVENT_CNTRL_CANCEL:
            // If cancelling, set to disabled state
            evt->state = EVENT_STATE_DISABLED;
            return curState;
        case EVENT_CNTRL_ENQUEUE:
            // If enqueing, only restart if currently disabled.
            if (!EVENT_STATE_ISDISABLED(curState)) {
                // If not, then treat as checking
                break;
            }
            // If state is disabled, fall-through
            FALLTHROUGH;
            //no break
        case EVENT_CNTRL_RESTART:
            // If forcing restart, set state to pending and current time
            if (evt->type != EVENT_TYPE_DISABLED) {
                // Set the event to pending if not disabled type
                evt->lastTime = curTime;
                if (interval != EVENT_INTERVAL_UNCHANGED) {
                    evt->interval = interval;
                }
                evt->state = EVENT_STATE_PENDING;
            }
            return curState;
        case EVENT_CNTRL_CHECK:
            // if just checking, skip any already in disabled state
            if (EVENT_STATE_ISDISABLED(curState)) {
                return curState;
            }
            // Do checks
            break;
        default:
            break;
    }
    // Event is not in disabled state. Handle checks
    if (evt->type == EVENT_TYPE_MANUAL) {
        // Check if the task has reached its interval
        _Event_checkOccured(evt, curTime);
        // If the event has occurred, update its state
        if (curState == EVENT_STATE_OCCURRED) {
            if (op == EVENT_CNTRL_ENQUEUE) {
                // Start if mode is to enqueue
                evt->lastTime = curTime;
                if (interval != EVENT_INTERVAL_UNCHANGED) {
                    evt->interval = interval;
                }
                evt->state = EVENT_STATE_PENDING;
            } else {
                // Otherwise now in disabled state.
                evt->state = EVENT_STATE_DISABLED;
            }
        }
    } else {
        // Update the state of the timer
        if (evt->type == EVENT_TYPE_DISABLED) {
            // Event state now disabled if the event type was changed to disabled
            evt->state = EVENT_STATE_DISABLED;
        } else {
            // Now pending otherwise.
            evt->state = EVENT_STATE_PENDING;
        }
    }
    return curState;
}

// Change mode for a timer event
//  - Pass in a timer event context returned from createEvent.
//  - Setting an interval of EVENT_INTERVAL_UNCHANGED (0) means keep the
//    previously set interval.
HpsErr_t Event_setMode(PEvent_t evt, EventType type, unsigned int interval) {
    // Ensure event valid
    if (!Event_validate(evt)) return ERR_NOTFOUND;
    // Update the type and optionally the interval
    evt->type = type;
    if (interval != EVENT_INTERVAL_UNCHANGED) {
        evt->interval = interval;
    }
    return ERR_SUCCESS;
}


