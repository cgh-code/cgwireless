/*
 * cgwireless.c
 *
 * Created: 02-06-2020
 * Author : Chris G Hough
 */ 

#ifndef F_CPU				// if F_CPU was not defined in Project -> Properties
#define F_CPU 1000000UL		// define it now as 8 MHz unsigned long
#endif

#define ADMUX_AREF			0x00
#define ADMUX_AVCC			0x40
#define ADMUX_INTERAL_1V	0xC0
#define ADMUX_10_BITS		0x00
#define ADMUX_8_BITS		0x20
#define ADC_PRESCALER_2		0x01
#define ADC_PRESCALER_4		0x02
#define ADC_PRESCALER_8		0x03
#define ADC_PRESCALER_16	0x04
#define ADC_PRESCALER_32	0x05
#define ADC_PRESCALER_64	0x06
#define ADC_PRESCALER_128	0x07

#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "cgoled.h"
#include "nrf24l01.h"
#include "cgrf.h"
#include "display.h"
#include "debug.h"

void setup_btn_interrupts();
void setup_led(void);
void setup_light_sensor();
void led_on(void);
void led_off(void);
void single_adc_conversion();

void config_transmit();
void run_transmit();
void config_receive();
void run_receive();
uint8_t find_channel();

volatile uint8_t m_button_on = 0;

// routine for PCMSK1 interrupt.
ISR(PCINT1_vect)
{
	// button pressed (low)
	if (!(PINC & (1 << PINC5)))
	{
		if (m_button_on == 0)
			m_button_on = 1;
		else
			m_button_on = 0;
	}
}

void setup_btn_interrupts()
{
	// button as input.
	DDRC &= ~(1 << DDC5);

	// PCMSK1 - Pin change mask register 1  (PCINT8 to PCINT14)
	// bit 5 for PCINT13.
	// PCIE1 bit in PCICR is set for PCMSK1.
	// call global sei routine to start interrupts.
	
	PCMSK1 |= (1<<PCINT13);
	PCICR |= (1<<PCIE1);
	sei();
}

void setup_led(void)
{
	// setup led for output.
	DDRC |= (1 << DDC4);
}

// switch led on.
void led_on(void)
{
	PORTC |= (1 << PORTC4);
}

// switch led off.
void led_off(void)
{
	PORTC &= ~(1 << PORTC4);
}

// setup light sensor as analogue input (PC3 / ADC3).
void setup_light_sensor()
{
	// configure port C light sensor as input.
	DDRC &= ~(1 << DDC3);
	
	// ADMUX - Multiplexer selection register.
	//
	// Use AVCC as voltage reference.
	// ADLAR bit set to left adjusted for 8 bit results.
	// MUX binary value of 0011 (0x03) set for ADC3 as analogue input.
	ADMUX |= ADMUX_AVCC | ADMUX_8_BITS | 0x03;

	// ADCSRA - Control and status register A.
	//
	// The ADEN bit (1 << 7) must be set to 1 to do any ADC operations.
	//
	// AVR ADC must be clocked at the frequency between 50 and 200kHz. 
	// So we need to set proper prescaller bits so that scaled system clock would fit in this range. 
	// Examples
	// --------
	// for AVR clocked at 16MHz, use 128 scaling factor which gives 16000000/128 = 125kHz of ADC clock.
	// for AVR clocked at 8MHz, use 64 scaling factor which gives 8000000/64 = 125kHz of ADC clock.
	// for AVR clocked at 1MHz, use 64 scaling factor which gives 1000000/8 = 125kHz of ADC clock.

	ADCSRA |= (1 << 7) | ADC_PRESCALER_8;
}

void single_adc_conversion()
{
	// Start an ADC conversion by setting ADSC bit (bit 6)
	ADCSRA |= (1 << ADSC);
	
	// Wait until the ADSC bit has been cleared
	while(ADCSRA & (1 << ADSC));
}

int main(void)
{
	//config_transmit();
	//run_transmit();

	config_receive();
	//find_channel();
	run_receive();
}

void config_transmit()
{
	setup_btn_interrupts();
	setup_led();
	setup_light_sensor();

	cgrf_init();
	cgrf_start_as_transmitter();
	cgrf_power_down();
	_delay_ms(5);
}

void run_transmit()
{
	uint8_t buffer[32] = {1, 2, 3};
	uint8_t running = 0;
	
	while (1)
	{
		if (running != m_button_on)
		{
			if (running == 0)
				running = 1;
			else
				running = 0;

			if (running == 1)
			{
				cgrf_power_up();
				led_on();
			}
			else
			{
				cgrf_power_down();
				led_off();
			}
		}
		
		if (running)
		{
			single_adc_conversion();
			buffer[0] = ADCH;
			buffer[1] = buffer[1] + 1;
			buffer[2] = 0;

			cgrf_transmit_data(&buffer[0], 3);
		}

		_delay_ms(50);
	}	
}

void config_receive()
{
	setup_btn_interrupts();
	setup_led();
	config_character_display();
	oled_power_on();

	cgrf_init();
	cgrf_start_as_reciever();
	led_on();
	_delay_ms(5);
}

void run_receive()
{
	uint8_t buffer[32] = {0, 0, 0};
	uint8_t running = 1;

	display_string("Listen",6,1,1);
	display_channel();
	m_button_on = 1;
	uint8_t div = 0;

	while (1)
	{
		if (running != m_button_on)
		{
			if (running == 0)
				running = 1;
			else
				running = 0;

			if (running == 1)
			{
				cgrf_power_up();
				led_on();
				oled_power_on();	
			}
			else
			{
				cgrf_power_down();
				led_off();
				oled_power_off();
			}
		}
		
		if (running == 1)
		{
			if (cgrf_data_ready() == 1)
			{		
				cgrf_get_payload(&buffer[0], 3);	
				div += 1;
			}
			
			// slow the display down.
			if (div == 4)
			{
				div = 0;
				display_number(buffer[0], 1, 2);
			}
		}
	}
}


uint8_t find_channel()
{
	uint8_t carrier = 0;
	uint8_t ch = 0;
	display_string("          ", 10,1,1);
	cgrf_set_crc_encoding(crc_none);
	
	while (1)
	{
		for (ch = 0; ch != 128; ch++)
		{
			cgrf_set_channel(ch);
			display_number(ch, 1, 1);
				
			for (uint8_t n = 0; n != 100; n++)
			{
				uint8_t status = nrf24_get_cd(&carrier);
				display_hex(status, 5, 1);
				
				if (carrier != 0)
					break;

				_delay_us(10);
			}
		
			if (carrier != 0)
				break;
		}
		
		if (carrier != 0)
			break;
	}

	cgrf_set_crc_encoding(crc_1_byte);
	
	display_string("found", 1, 1, 1);
	display_channel();
	display_binary(carrier, 1, 2);
	
	return carrier;
}
