/*
 * button.c
 *
 * Created: 26/06/2020 10:35:23
 *  Author: Chris
 */ 

#include "button.h"
#include <avr/io.h>

// buttons.
#define BTN0 PC5

void config_buttons(void)
{
	// setup buttons for input.
	DDRC &= ~(1 << BTN0);
}

// returns true when button 1 is pressed.
bool button1_down(void)
{
	return !(PINC & (1 << BTN0));
}