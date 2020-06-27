/*
 * display.c
 *
 * Created: 26/06/2020 10:25:33
 *  Author: Chris
 */ 

#include "display.h"
#include "cgoled.h"

// configures the display to: -
// 2 rows of characters.
// characters size of 5 x 7 pixels.
// use 8 bit mode (requires 8 pins for databus).
// auto increment cursor position after writing a character or pixels.
// switches the display to graphics mode which makes the character modes irrelevant until mode is switched.
void config_character_display(void)
{
	oled_config();
	oled_write_cmd(CMD_FUNC_CONTROL | CMD_FUNC_8BIT | CMD_FUNC_2LINES);
	oled_cursor_home();
	oled_incremental_cursor();
	oled_clear();
}

// configures the display to: -
// 2 rows of characters.
// characters size of 5 x 7 pixels.
// use 8 bit mode (requires 8 pins for databus).
// auto increment cursor position after writing a character or pixels.
// switches the display to graphics mode which makes the character modes irrelevant until mode is switched.
void config_graphical_display(void)
{
	oled_config();
	oled_write_cmd(CMD_FUNC_CONTROL | CMD_FUNC_8BIT | CMD_FUNC_2LINES);
	oled_cursor_home();
	oled_incremental_cursor();
	oled_graphics_mode();
	oled_clear();
}


void display_number(uint8_t const n, uint8_t const x, uint8_t const y)
{
	uint8_t rem = n;
	uint8_t dig = rem / 100;
	
	oled_write_character(0x30 + dig, x, y);
	rem -= dig * 100;
	
	dig = rem / 10;
	oled_write_character(0x30 + dig, x+1, y);
	rem -= dig * 10;

	oled_write_character(0x30 + rem, x+2, y);
}

void display_hex(uint8_t const n, uint8_t const x, uint8_t const y)
{
	uint8_t hex[16] = { 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46 };
	uint8_t ch1 = hex[(n >> 4)];
	uint8_t ch2 = hex[(n & 0x0F)];
	
	oled_write_character(ch1, x, y);
	oled_write_character(ch2, x+1, y);
}

void display_binary(uint8_t const n, uint8_t const x, uint8_t const y)
{
	uint8_t xpos = x;
	
	for (uint8_t i = 0; i !=8; i++)
	{
		if (n & (1 << (7 - i)))
		{
			oled_write_character('1', xpos, y);
		}
		else
		{
			oled_write_character('0', xpos, y);
		}
		
		xpos++;
	}
}

void display_string(char * const text, uint8_t const size, uint8_t const x, uint8_t const y)
{
	uint8_t xpos = x;
	char * ptr = text;
	
	for (uint8_t i = 0; i !=size; i++)
	{
		char ch = *ptr;
		uint8_t n = (uint8_t)ch;
		oled_write_character(n, xpos, y);
		xpos++;
		ptr++;
	}
}