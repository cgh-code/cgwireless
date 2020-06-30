/*
 * debug.c
 *
 * Created: 30-06-2020
 * Author:  Chris Hough
 */ 

#ifndef F_CPU				// if F_CPU was not defined in Project -> Properties
#define F_CPU 1000000UL		// define it now as 8 MHz unsigned long
#endif

#include "debug.h"
#include "display.h"
#include "nrf24l01.h"
#include "cgoled.h"
#include <util/delay.h>

void display_channel()
{
	uint8_t ch = 0;
	nrf24_get_rf_ch(&ch);
	display_number(ch, 8, 1);
}

void display_payload_size()
{
	uint8_t plsize = 0;
	nrf24_get_payload_size(&plsize);
	display_number(plsize, 1, 1);
}

void display_buffer_hex(uint8_t const * const buffer)
{
	display_hex(buffer[0], 1, 2);
	oled_write_character(0x20, 3, 1);
	display_hex(buffer[1], 4, 2);
	oled_write_character(0x20, 6, 1);
	display_hex(buffer[2], 7, 2);
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

