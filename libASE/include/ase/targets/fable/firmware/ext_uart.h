/*
 * ext_uart.h
 *
 * Created: 12/10/2012 14:17:31
 *  Author: Walteruzzu
 */ 


#ifndef EXT_UART_H_
#define EXT_UART_H_

#define EXT_UART_0 (0xd8)
#define EXT_UART_1 (0xb8)
#define EXT_UART_2 (0x58)
#define EXT_UART_3 (0x38)

#include <avr/io.h>
#include "macro.h"
#include "i2c.h"

static const uint8_t ext_uart_channels[4]={ EXT_UART_0, EXT_UART_1, EXT_UART_2, EXT_UART_3};

typedef void (*ext_uart_int_callback)(void);

void ext_uart_init();
void ext_uart_enable_int_rx(uint8_t level,const ext_uart_int_callback callback);

void ext_uart_loop_off(uint8_t channel);
void ext_uart_loop_on(uint8_t channel);

void ext_uart_send(uint8_t addr, uint8_t* message, uint8_t length);

void ext_uart_0_send(uint8_t byte);
void ext_uart_1_send(uint8_t byte);
void ext_uart_2_send(uint8_t byte);
void ext_uart_3_send(uint8_t byte);

uint8_t ext_uart_0_rx(void);
uint8_t ext_uart_1_rx(void);
uint8_t ext_uart_2_rx(void);
uint8_t ext_uart_3_rx(void);

uint8_t ext_uart_0_poll(void);
uint8_t ext_uart_1_poll(void);
uint8_t ext_uart_2_poll(void);
uint8_t ext_uart_3_poll(void);

#endif /* EXT_UART_H_ */
