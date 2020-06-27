/*
 * cgoled.c
 *
 * Created: 2020
 * Author:  Chris Hough
 *
 * Functions that provide access to the capabilities of the MC0010 controller,
 * used in the Midas and Vishay OLED displays.
 *
 * https://uk.farnell.com/midas/mcob050016av-bp/display-oled-graphic-cob-50x16/dp/2769654?CMP=i-bf9f-00001000
 * https://uk.farnell.com/vishay/o100h016egpp5n0000/display-oled-graphic-100x16-pixels/dp/2769925?CMP=i-bf9f-00001000
 *
 */

#include "cgoled.h"
#include <avr/io.h>

#ifndef F_CPU				// if F_CPU was not defined in Project -> Properties
#define F_CPU 1000000UL		// define it now as 1 MHz unsigned long
#endif


// Note. Example addresses below are hex.

// ***** DISPLAYS MORE THAN 40 x 2 Lines *****
// ***** CASE N0                         *****
//
// Display Position ->  1   2   3   4   5   6   7   8
// --------------------------------------------------
// DDRAM Address ->    00, 01, 02, 03, 04, 05, 06, 07

// After shift left.....
// Display Position ->  1   2   3   4   5   6   7   8
// --------------------------------------------------
// DDRAM Address ->    01, 02, 03, 04, 05, 06, 07, 08

// After shift right.....
// Display Position ->  1   2   3   4   5   6   7   8
// --------------------------------------------------
// DDRAM Address ->    4F, 00, 01, 02, 03, 04, 05, 06


// ***** DISPLAYS LESS THAN 40 x 2 Lines *****
// ***** CASE N1                         *****
//
// Display Position ->      1   2   3   4   5   6   7   8      39  40
// ------------------------------------------------------------------
// DDRAM Address Line 1 -> 00, 01, 02, 03, 04, 05, 06, 07 .... 26, 27
// DDRAM Address Line 2 -> 40, 41, 42, 43, 44, 45, 46, 47 .... 66, 67

// *After shift left*
// Display Position ->      1   2   3   4   5   6   7   8
// ------------------------------------------------------
// DDRAM Address Line 1 -> 01, 02, 03, 04, 05, 06, 07, 08
// DDRAM Address Line 2 -> 41, 42, 43, 44, 45, 46, 47, 48

// *After shift right*
// Display Position ->      1   2   3   4   5   6   7   8
// ------------------------------------------------------
// DDRAM Address Line 1 -> 27, 00, 01, 02, 03, 04, 05, 06
// DDRAM Address Line 2 -> 67, 40, 41, 42, 43, 44, 45, 46

// 0100 0000 (1 << 6)
#define CMD_CGRAM 0x40

// 1000 0000 (1 << 7)
#define CMD_DDRAM 0x80


// Cursor/Shift/Mode/Power	(1 << 4).
#define CMD_MODE 0x10

// Graphics Mode			(1 << 3).
#define CMD_MODE_GFX 0x08

// Internal power on		(1 << 2).
#define CMD_MODE_POWER 0x04

// For graphics				(1 << 1) | 1
#define CMD_MODE_GFX_FLAG 0x03


// private function declarations.
void busy_wait();
void set_data_bus(uint8_t data);
uint8_t get_ddram_address_n1(uint8_t column_n, uint8_t row_n);
uint8_t get_cgram_address(uint8_t char_n, uint8_t row_n);
uint8_t get_gxa_address(uint8_t x);
uint8_t get_gya_address(uint8_t cga);

// Sets the ports using the defines declared in the header file.
void oled_config()
{
	// setup port D pins for output.
	OLED_DDR_DB0 |= (1 << OLED_DB0);
	OLED_DDR_DB1 |= (1 << OLED_DB1);
	OLED_DDR_DB2 |= (1 << OLED_DB2);
	OLED_DDR_DB3 |= (1 << OLED_DB3);
	OLED_DDR_DB4 |= (1 << OLED_DB4);
	OLED_DDR_DB5 |= (1 << OLED_DB5);
	OLED_DDR_DB6 |= (1 << OLED_DB6);
	OLED_DDR_DB7 |= (1 << OLED_DB7);

	// setup Port B pins for output.
	OLED_DDR_RS |= (1 << OLED_RS);
	OLED_DDR_RW |= (1 << OLED_RW);
	OLED_DDR_EN |= (1 << OLED_EN);
}

// clears the display using the hardware feature.
// seems a little slow.
void oled_clear()
{
	oled_write_cmd(CMD_CLEAR_DISPLAY);
}

// fills the entire display with off pixels to clear the display.
void oled_blank()
{
	for (uint8_t row = 0; row != OLED_BYTE_ROWS; ++row)
	{
		oled_set_coordinates(1, row + 1);

		for (uint8_t i = 0; i != OLED_PIXEL_COLUMNS; i++)
		{
			oled_write_data(0x00);
		}		
	}
}

// sets the cursor back to home (top left).
void oled_cursor_home()
{
	oled_write_cmd(CMD_CURSOR_HOME);
}

// Switch to incremental cursor mode.
void oled_incremental_cursor()
{
	oled_write_cmd(CMD_ENTRY_CONTROL | CMD_ENTRY_INCREMENT);
}

// switch to graphics mode.
void oled_graphics_mode()
{
	oled_write_cmd(CMD_MODE | CMD_MODE_GFX | CMD_MODE_POWER | CMD_MODE_GFX_FLAG);
}


// switch the OLED on.
void oled_power_on()
{
	oled_write_cmd(CMD_DISPLAY_CONTROL | CMD_DISPLAY_POWER);
}

// switch the OLED on.
void oled_power_off()
{
	oled_write_cmd(CMD_DISPLAY_CONTROL);
}

// write character at given position.
// column and row are 1 based.
void oled_write_character(uint8_t character, uint8_t column, uint8_t row)
{
	uint8_t addr = get_ddram_address_n1(column, row);
	oled_write_cmd(CMD_DDRAM | addr);

	oled_write_data(character);
}


// sets a user defined character in the displays CGRAM.
// 8 user definable characters (char_n 1 to 8).
// characters are 5x8 (7 + cursor row).
// patterns pointer must point to 8 uint8_t rows.
void oled_set_character(uint8_t char_n, uint8_t const * const patterns)
{
	uint8_t addr;
	uint8_t ptn;

	for (uint8_t n = 0; n != 7; n++)
	{
		addr = get_cgram_address(char_n, n + 1);
		oled_write_cmd(CMD_CGRAM | addr);

		ptn = *(patterns + n);
		ptn |= (1 << 7) | (1 << 6) | (1 << 5);
		oled_write_data(ptn);
	}
}


// Writes an operation (display clear etc.). Checks the busy flag first.
void oled_write_cmd(uint8_t command)
{
	oled_write_cmd_busy(command, true);
}

// Writes an operation (display clear etc.).  Optionally checks the busy flag first.
void oled_write_cmd_busy(uint8_t command, bool wait_for_bf)
{
	if (wait_for_bf)
		busy_wait();

	// Set the data bus.
	set_data_bus(command);

	// 0 - command register.
	OLED_PORT_RS &= ~(1 << OLED_RS);

	// 0 - write.
	OLED_PORT_RW &= ~(1 << OLED_RW);

	// Pulse the enable. (on, off)
	OLED_PORT_EN |= (1 << OLED_EN);
	OLED_PORT_EN &= ~(1 << OLED_EN);
}


// Writes the given data to DDRAM or CGRAM.
void oled_write_data(uint8_t data)
{
	busy_wait();

	 // Set the data bus.
	set_data_bus(data);

	// 1 - data register.
	OLED_PORT_RS |= (1 << OLED_RS);

	// 0 - write.
	OLED_PORT_RW &= ~(1 << OLED_RW);

	 // Pulse the enable. (on, off)
	OLED_PORT_EN |= (1 << OLED_EN);
	OLED_PORT_EN &= ~(1 << OLED_EN);
}

// Set the x and y coordinates for graphics.  Top left is 1,1.
// note:  the cy co-ordinate is multiple of 8 pixels.
// eg. cy:1  y=1,
//     cy:2  y=9.
void oled_set_coordinates(uint8_t x, uint8_t cy)
{
	uint8_t gxa = get_gxa_address(x);
	uint8_t gya = get_gya_address(cy);

	oled_write_cmd(gxa);
	oled_write_cmd(gya);
}

// write pixels at the given x and y co-ordinates.
// note:  the cy co-ordinate is multiple of 8 pixels.
void oled_write_pixels_at(uint8_t x, uint8_t cy, uint8_t pixels)
{
	oled_set_coordinates(x, cy);
	oled_write_data(pixels);
}


// Reads the busy flag until the display becomes available for another instruction.
void busy_wait()
{
	// Set data bus bit 7 as input.
	OLED_DDR_DB7 &= ~(1 << OLED_DB7);

	// 0 - command register.
	OLED_PORT_RS &= ~(1 << OLED_RS);

	// 1 - read.
	OLED_PORT_RW |= (1 << OLED_RW);

	// read busy flag until it is 0 (not busy).
	do
	{
		OLED_PORT_EN |= (1 << OLED_EN);
		OLED_PORT_EN &= ~(1 << OLED_EN);

	} while (OLED_PIN_DB7 & (1 << OLED_DB7));

	// restore data bus bit 7 as output.
	OLED_DDR_DB7 |= (1 << OLED_DB7);

	// 0 - write.
	OLED_PORT_RW &= ~(1 << OLED_RW);
}

// Sets the data registers to the given data.
void set_data_bus(uint8_t data)
{
	if (data & (1 << 7))
		OLED_PORT_DB7 |= (1 << OLED_DB7);
	else
		OLED_PORT_DB7 &= ~(1 << OLED_DB7);

	if (data & (1 << 6))
		OLED_PORT_DB6 |= (1 << OLED_DB6);
	else
		OLED_PORT_DB6 &= ~(1 << OLED_DB6);

	if (data & (1 << 5))
		OLED_PORT_DB5 |= (1 << OLED_DB5);
	else
		OLED_PORT_DB5 &= ~(1 << OLED_DB5);

	if (data & (1 << 4))
		OLED_PORT_DB4 |= (1 << OLED_DB4);
	else
		OLED_PORT_DB4 &= ~(1 << OLED_DB4);

	if (data & (1 << 3))
		OLED_PORT_DB3 |= (1 << OLED_DB3);
	else
		OLED_PORT_DB3 &= ~(1 << OLED_DB3);

	if (data & (1 << 2))
		OLED_PORT_DB2 |= (1 << OLED_DB2);
	else
		OLED_PORT_DB2 &= ~(1 << OLED_DB2);

	if (data & (1 << 1))
		OLED_PORT_DB1 |= (1 << OLED_DB1);
	else
		OLED_PORT_DB1 &= ~(1 << OLED_DB1);

	if (data & 1)
		OLED_PORT_DB0 |= (1 << OLED_DB0);
	else
		OLED_PORT_DB0 &= ~(1 << OLED_DB0);
 }

// gets the address for the given column and row.
// displays using case N1. (see comments at top).
uint8_t get_ddram_address_n1(uint8_t column_n, uint8_t row_n)
{
	uint8_t addr = 0x00;

	if (row_n == 2)
	{
		addr = 0x40;
	}

	if (column_n > 1 && column_n <= 40)
	{
		addr |= (column_n - 1);
	}

	return addr;
}

uint8_t get_cgram_address(uint8_t char_n, uint8_t row_n)
{
	uint8_t addr = 0x00;

	if (char_n > 1 && char_n <= 8)
	{
		addr |= ((char_n - 1) << 3);
	}

	if (row_n > 1 && row_n <= 8)
	{
		addr |= (row_n - 1);
	}

	return addr;
}

uint8_t get_gxa_address(uint8_t x)
{
	// 1000 0000 (1 << 7)
	uint8_t addr = 0x80;

	if (x > 1 && x <= OLED_PIXEL_COLUMNS)
	{
		addr |= (x - 1);
	}

	return addr;
}

uint8_t get_gya_address(uint8_t cga)
{
	// 0100 0000 (1 << 6)
	uint8_t addr = 0x40;

	if (cga == 2)
	{
		addr |= 1;
	}

	return addr;
}