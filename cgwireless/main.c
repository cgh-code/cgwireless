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
#include "cgrf.h"
#include "led.h"
#include "button.h"
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

int main(void)
{
	//config_transmit();
	//run_transmit();
		
	config_receive();
	run_receive();
}

void config_transmit()
{
	config_buttons();
	config_led();
	//config_character_display();
	oled_power_on();

	cgrf_init();
	cgrf_start_as_transmitter();
	cgrf_power_down();
	_delay_ms(5);	
}

void run_transmit()
{
	uint8_t buffer[32] = {1, 2, 3};
	acknowledgment_t ack = success;
	uint8_t running = 0;
	uint8_t sent = 0;
	uint8_t failed = 0;
	
	while (1)
	{
		if (button1_down())
		{
			if (running == 0)
			{
				running = 1;
				cgrf_power_up();
				led_on();
			}
			else
			{
				running = 0;
				cgrf_power_down();
				led_off();
			}
			
			_delay_ms(100);
		}
		
		if (running)
		{
			ack = cgrf_transmit_data(&buffer[0], 3);
			
			if (ack == success)
			{
				increment_data(buffer);
				sent++;
			}
			else
			{
				failed++;
			}
		}

		//display_string("STATUS", 6, 1, 1);
		//display_number((uint8_t)ack, 8, 1);
		//display_number(sent, 1, 2);
		//display_number(failed, 5, 2);
		_delay_ms(200);
	}	
}

void config_receive()
{
	config_buttons();
	config_led();
	config_character_display();
	oled_power_on();

	cgrf_init();
	cgrf_start_as_reciever();
	_delay_ms(5);
}

void run_receive()
{
	uint8_t buffer[32] = {0, 0, 0};

	uint8_t running = 1;
	uint8_t received = 0;
	uint8_t failed = 0;
	uint8_t status = 0;

	if (running != 0)
		led_on();

	//display_registers();
	oled_clear();

	while (1)
	{
		if (button1_down())
		{
			if (running == 0)
			{
				running = 1;
				cgrf_power_up();
				led_on();
				oled_power_on();
			}
			else
			{
				running = 0;
				cgrf_power_down();
				led_off();
				oled_power_off();
			}
			
			_delay_ms(100);
		}
		
		if (running == 1)
		{
			status = cgrf_get_payload(&buffer[0], 3);
			
			display_binary(status, 1, 1);
			display_number(buffer[0], 1, 2);
			display_number(buffer[1], 5, 2);
			display_number(buffer[2], 8, 2);
			_delay_ms(50);
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

