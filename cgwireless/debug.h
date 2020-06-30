/*
 * debug.h
 *
 * Created: 30/06/2020 10:06:59
 *  Author: Chris
 */ 

#include <stdint.h>

#ifndef DEBUG_H_
#define DEBUG_H_

void display_channel();
void display_payload_size();
void display_buffer_hex(uint8_t const * const buffer);
void display_register(char * const text, uint8_t const size, uint8_t value);
void display_address(uint8_t const * const addr);
void display_registers();

#endif /* DEBUG_H_ */