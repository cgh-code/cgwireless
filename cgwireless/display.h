/*
 * display.h
 *
 * Created: 26/06/2020 10:25:20
 *  Author: Chris
 */ 

#include <stdint.h>

#ifndef DISPLAY_H_
#define DISPLAY_H_

void config_character_display(void);
void config_graphical_display(void);

void display_number(uint8_t const n, uint8_t const x, uint8_t const y);
void display_hex(uint8_t const n, uint8_t const x, uint8_t const y);
void display_binary(uint8_t const n, uint8_t const x, uint8_t const y);
void display_string(char * const text, uint8_t const size, uint8_t const x, uint8_t const y);

#endif /* DISPLAY_H_ */