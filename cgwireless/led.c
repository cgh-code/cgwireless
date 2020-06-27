/*
 * led.c
 *
 * Created: 26/06/2020 10:29:38
 *  Author: Chris
 */ 

#include "led.h"
#include <avr/io.h>

#define LED0 PC4

void config_led(void)
{
	// setup led for output.
	DDRC |= (1 << LED0);
}

// switch led on.
void led_on(void)
{
	PORTC |= (1 << LED0);
}

// switch led off.
void led_off(void)
{
	PORTC &= ~(1 << LED0);
}