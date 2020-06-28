/*
 * cgwireless.c
 *
 * Created: 02-06-2020
 * Author : Chris G Hough
 */ 

#ifndef F_CPU				// if F_CPU was not defined in Project -> Properties
#define F_CPU 1000000UL		// define it now as 8 MHz unsigned long
#endif

#include <avr/io.h>
#include <stdint.h>
#include <stdbool.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include "cgrf.h"
#include "led.h"
#include "display.h"
#include "cgoled.h"
#include "nrf24l01.h"

void config_transmit();
void run_transmit();
void config_receive();
void run_receive();

void increment_data(uint8_t data[5]);
void display_register(char * const text, uint8_t const size, uint8_t value);
void display_address(uint8_t const * const addr);
void display_registers();
void setup_btn_interrupts();

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

int main(void)
{
	//config_transmit();
	//run_transmit();

	config_receive();
	run_receive();
}

void config_transmit()
{
	setup_btn_interrupts();
	config_led();

	cgrf_init();
	cgrf_start_as_transmitter();
	cgrf_power_down();
	_delay_ms(5);
}

void run_transmit()
{
	uint8_t buffer[32] = {1, 2, 3};
	uint8_t running = 0;
	acknowledgment_t ack = success;
	
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
			ack = cgrf_transmit_data(&buffer[0], 3);
		
			if (ack == success)
				increment_data(buffer);
		}

		_delay_ms(50);
	}	
}

void config_receive()
{
	setup_btn_interrupts();
	config_led();
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
	uint8_t status = 0;

	display_string("Listening",9,1,1);
	m_button_on = 1;

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
				status = cgrf_get_payload(&buffer[0], 3);
				
				display_binary(status, 1, 1);
				display_string(" ", 1, 9, 1);
				display_number(buffer[0], 1, 2);
				display_number(buffer[1], 5, 2);
				display_number(buffer[2], 8, 2);
			}
	
		}
	}
}


void increment_data(uint8_t data[5])
{
	uint8_t n = data[2];
	
	if (n >= 250)
		n = 0;
	
	data[0] = ++n;
	data[1] = ++n;
	data[2] = ++n;
}

void display_register(char * const text, uint8_t const size, uint8_t value)
{
	display_string(text, size, 1, 1);
	display_binary(value, 1, 2);
	_delay_ms(1000);
}

void display_address(uint8_t const * const addr)
{
	for (uint8_t i = 0; i != 5; i++)
	{
		display_hex(addr[i], i*2+1, 2);
	}
}

void display_registers()
{
	uint8_t value = 0;
	
	value = 0;
	nrf24_get_config(&value);
	display_register("CONFIG    ", 10, value);

	value = 0;
	nrf24_get_en_aa(&value);
	display_register("EN AA     ", 10, value);

	value = 0;
	nrf24_get_en_rxaddr(&value);
	display_register("EN RXADDR ", 10, value);

	value = 0;
	nrf24_get_setup_aw(&value);
	display_register("SETUP AW  ", 10, value);

	value = 0;
	nrf24_get_setup_retr(&value);
	display_register("SETUP RETR", 10, value);

	value = 0;
	nrf24_get_rf_ch(&value);
	display_register("RF CH     ", 10, value);

	value = 0;
	nrf24_get_rf_setup(&value);
	display_register("RF SETUP  ", 10, value);

	value = 0;
	nrf24_get_status(&value);
	display_register("STATUS    ", 10, value);

	value = 0;
	nrf24_get_observe_tx(&value);
	display_register("OBSERVE TX", 10, value);

	value = 0;
	nrf24_get_rx_pw_p0(&value);
	display_register("RX PW P0  ", 10, value);

	value = 0;
	nrf24_get_rx_pw_p1(&value);
	display_register("RX PW P1  ", 10, value);

	value = 0;
	nrf24_get_fifo_status(&value);
	display_register("FIFO STAT ", 10, value);

	value = 0;
	nrf24_get_dynpd(&value);
	display_register("DYNPD     ", 10, value);

	value = 0;
	nrf24_get_feature(&value);
	display_register("FEATURE   ", 10, value);


	uint8_t addr[5] = { 0x00, 0x00, 0x00, 0x00, 0x00 };
	
	nrf24_get_tx_address(&addr[0]);
	display_string("TXADDR    ", 10, 1, 1);
	display_address(&addr[0]);
	_delay_ms(1000);
	
	nrf24_get_rx_address_pipe0(&addr[0]);
	display_string("RX ADDR P0", 10, 1, 1);
	display_address(&addr[0]);
	_delay_ms(1000);

	nrf24_get_rx_address_pipe1(&addr[0]);
	display_string("RX ADDR P1", 10, 1, 1);
	display_address(&addr[0]);
	_delay_ms(1000);
}

