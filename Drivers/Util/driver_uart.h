/* Generic UART Driver Interface
 * -----------------------------
 *
 * Provides a common interface for different UART
 * drivers to allow them to be used as a generic
 * handler.
 *
 * Company: University of Leeds
 * Author: T Carpenter
 *
 * Change Log:
 *
 * Date       | Changes
 * -----------+----------------------------------
 * 08/01/2024 | Creation of driver.
 */

#ifndef DRIVER_UART_H_
#define DRIVER_UART_H_

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include <Util/driver_ctx.h>

#include "Util/error.h"

//8-bit variant of Rx Data structure
typedef struct {
    uint8_t rxData;       //Data byte
    bool    partityError; //Indicates that this data byte suffered a parity error
    bool    frameError;   //Indicates that this data byte suffered a framing error
    bool    valid;        //Indicates that the FIFO had a data byte when a read was attempted
} UartRxData_t;

//16-bit variant of Rx Data Structure
typedef struct {
    uint16_t rxData;       //Data word
    bool     partityError; //Indicates that this data byte suffered a parity error
    bool     frameError;   //Indicates that this data byte suffered a framing error
    bool     valid;        //Indicates that the FIFO had a data byte when a read was attempted
} UartRxDataExt_t;

enum {
    UART_BAUD_MIN     = 1      ,
    UART_BAUD_4800    = 4800   ,
    UART_BAUD_9600    = 9600   ,
    UART_BAUD_19200   = 19200  ,
    UART_BAUD_25000   = 25000  ,
    UART_BAUD_38400   = 38400  ,
    UART_BAUD_50000   = 50000  ,
    UART_BAUD_57600   = 57600  ,
    UART_BAUD_115200  = 115200 ,
    UART_BAUD_125000  = 125000 ,
    UART_BAUD_230400  = 230400 ,
    UART_BAUD_250000  = 250000 ,
    UART_BAUD_1000000 = 1000000,
    UART_BAUD_2000000 = 2000000,
    UART_BAUD_2500000 = 2500000,
    UART_BAUD_MAX     = -1
};

typedef enum __attribute__ ((packed)) {
    UART_NOPARITY   = 0,
    UART_EVENPARITY = 1,
    UART_ODDPARITY  = 3
} UartParity;

enum {
    UART_3BIT = 3,
    UART_4BIT = 4,
    UART_5BIT = 5,
    UART_6BIT = 6,
    UART_7BIT = 7,
    UART_8BIT = 8,
    UART_9BIT = 9,
};


// IO Function Templates
typedef HpsErrExt_t (*UartTxRxFunc_t)     (void* ctx, uint8_t data[], uint8_t length);
typedef HpsErrExt_t (*UartFifoSpaceFunc_t)(void* ctx);
typedef HpsErr_t    (*UartFifoClearFunc_t)(void* ctx, bool tx, bool rx);
typedef HpsErrExt_t (*UartStatusFunc_t)   (void* ctx, bool clearFlag);

// GPIO Context
typedef struct {
    // Driver Context
    void* ctx;
    // Driver Function Pointers
    UartTxRxFunc_t transmit;
    UartTxRxFunc_t receive;
    // Status Functions
    UartStatusFunc_t txIdle;
    UartStatusFunc_t rxReady;
    // FIFO Functions
    UartFifoSpaceFunc_t    txFifoSpace;
    UartFifoSpaceFunc_t    rxFifoAvailable;
    UartFifoClearFunc_t    clearFifos;
} UartCtx_t, *PUartCtx_t;

// Check if driver initialised
inline HpsErr_t UART_isInitialised(PUartCtx_t uart) {
    if (!uart) return ERR_NULLPTR;
    return DriverContextCheckInit(uart->ctx);
}

// Check transmitter is idle
// - Non-negative return value indicates idle status flag value
// - If clearFlag is true, the status flag should be cleared
// - Negative indicates error
inline HpsErrExt_t UART_txIdle(PUartCtx_t uart, bool clearFlag) {
    if (!uart) return ERR_NULLPTR;
    if (!uart->txIdle) return ERR_NOSUPPORT;
    return uart->txIdle(uart->ctx, clearFlag);
}

// Check receiver has data ready
// - Non-negative return value indicates busy status flag value
// - If clearFlag is true, the status flag should be cleared
// - Negative indicates error
inline HpsErrExt_t UART_rxReady(PUartCtx_t uart, bool clearFlag) {
    if (!uart) return ERR_NULLPTR;
    if (!uart->rxReady) return ERR_NOSUPPORT;
    return uart->rxReady(uart->ctx, clearFlag);
}

// Check transmit data FIFO space
// - Non-negative return value indicates amount of space in FIFO for data
// - Negative indicates error
inline HpsErrExt_t UART_txFifoSpace(PUartCtx_t uart) {
    if (!uart) return ERR_NULLPTR;
    if (!uart->txFifoSpace) return ERR_NOSUPPORT;
    return uart->txFifoSpace(uart->ctx);
}

// Check receive data FIFO availability
// - Non-negative return value indicates amount of space in FIFO for data
// - Negative indicates error
inline HpsErrExt_t UART_rxFifoAvailable(PUartCtx_t uart) {
    if (!uart) return ERR_NULLPTR;
    if (!uart->rxFifoAvailable) return ERR_NOSUPPORT;
    return uart->rxFifoAvailable(uart->ctx);
}

// Clear FIFOs
// - Tx and Rx flags select which FIFOs to clear
// - Negative indicates error
inline HpsErr_t UART_clearFifos(PUartCtx_t uart, bool tx, bool rx) {
    if (!uart) return ERR_NULLPTR;
    if (!uart->clearFifos) return ERR_NOSUPPORT;
    return uart->clearFifos(uart->ctx, tx, rx);
}

// Transmit data
// - Positive return value indicates amount of data sent.
// - Negative indicates error.
inline HpsErrExt_t UART_transmit(PUartCtx_t uart, uint8_t data[], uint8_t length) {
    if (!length) return ERR_SUCCESS;
    if (!uart || !data) return ERR_NULLPTR;
    if (!uart->transmit) return ERR_NOSUPPORT;
    return uart->transmit(uart->ctx,data,length);
}

// Receive data
// - Positive return value indicates amount of data received.
// - Negative indicates error.
inline HpsErrExt_t UART_receive(PUartCtx_t uart, uint8_t data[], uint8_t length) {
    if (!length) return ERR_SUCCESS;
    if (!uart || !data) return ERR_NULLPTR;
    if (!uart->receive) return ERR_NOSUPPORT;
    return uart->receive(uart->ctx,data,length);
}

#endif /* DRIVER_UART_H_ */
