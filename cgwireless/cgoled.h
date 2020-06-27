/*
 * cgoled.h
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

#include <stdint.h>
#include <stdbool.h>

#ifndef CGOLED_H_
#define CGOLED_H_

// user definable data direction register mapping (input or output).
#define OLED_DDR_DB0 DDRD
#define OLED_DDR_DB1 DDRD
#define OLED_DDR_DB2 DDRD
#define OLED_DDR_DB3 DDRD
#define OLED_DDR_DB4 DDRD
#define OLED_DDR_DB5 DDRD
#define OLED_DDR_DB6 DDRD
#define OLED_DDR_DB7 DDRD
#define OLED_DDR_RS DDRB
#define OLED_DDR_RW DDRB
#define OLED_DDR_EN DDRB

// user definable port mapping.
#define OLED_PORT_DB0 PORTD
#define OLED_PORT_DB1 PORTD
#define OLED_PORT_DB2 PORTD
#define OLED_PORT_DB3 PORTD
#define OLED_PORT_DB4 PORTD
#define OLED_PORT_DB5 PORTD
#define OLED_PORT_DB6 PORTD
#define OLED_PORT_DB7 PORTD
#define OLED_PORT_RS PORTB
#define OLED_PORT_RW PORTB
#define OLED_PORT_EN PORTB

// user definable port pin mapping for reading.
#define OLED_PIN_DB7 PIND

// user definable port pin mapping for writing.
#define OLED_DB0 PD0
#define OLED_DB1 PD1
#define OLED_DB2 PD2
#define OLED_DB3 PD3
#define OLED_DB4 PD4
#define OLED_DB5 PD5
#define OLED_DB6 PD6
#define OLED_DB7 PD7
#define OLED_RS PB2
#define OLED_RW PB1
#define OLED_EN PB0

// Set the pixel columns and row to match the OLED display.
#define OLED_PIXEL_COLUMNS 50
#define OLED_PIXEL_ROWS    16
#define OLED_BYTE_ROWS (OLED_PIXEL_ROWS / 8)

// Command bits used to control the OLED.
// Used as arguments when calling oled_write_cmd().
//
// Examples.
//
// OLED auto increments the cursor position after writing a character.
// oled_write_cmd(CMD_ENTRY_CONTROL | CMD_ENTRY_INCREMENT);
//
// Moves the entire display left by one character creating a right scrolling effect.
// oled_write_cmd(CMD_SHIFT_CONTROL | CMD_SHIFT_DISPLAY);
//
// Moves the entire display right by one character creating a left scrolling effect.
// oled_write_cmd(CMD_SHIFT_CONTROL | CMD_SHIFT_DISPLAY | CMD_SHIFT_RIGHT);
//
// Moves the cursor position right by one character.
// oled_write_cmd(CMD_SHIFT_CONTROL | CMD_SHIFT_RIGHT);
//
// *Warning!* - Shifting effects the DDRAM address.
//

// (1)
#define CMD_CLEAR_DISPLAY 0x01

// (1 << 1)
#define CMD_CURSOR_HOME 0x02

// (1 << 2)
#define CMD_ENTRY_CONTROL 0x4
#define CMD_ENTRY_INCREMENT 0x2
#define CMD_ENTRY_SHIFT_DISPLAY 0x1

// (1 << 3)
#define CMD_DISPLAY_CONTROL 0x08
#define CMD_DISPLAY_POWER 0x04
#define CMD_DISPLAY_CURSOR 0x02
#define CMD_DISPLAY_BLINK 0x01

// (1 << 4)
#define CMD_SHIFT_CONTROL 0x10
#define CMD_SHIFT_DISPLAY 0x8
#define CMD_SHIFT_RIGHT 0x4

// (1 << 5)
#define CMD_FUNC_CONTROL 0x20
#define CMD_FUNC_8BIT 0x10
#define CMD_FUNC_2LINES 0x08
#define CMD_FUNC_FONT_10X7 0x4
//#define CMD_GFX_MODE 0x2

// Sets the ports using the defines declared in the header file.
void oled_config();

// clears the display using the hardware feature.
// seems a little slow.
void oled_clear();

// fills the entire display with off pixels to clear the display.
void oled_blank();

// sets the cursor back to home (top left).
void oled_cursor_home();

// Switch to incremental cursor mode.
void oled_incremental_cursor();

// switch to graphics mode.
void oled_graphics_mode();

// switch the OLED on.
void oled_power_on();

// switch the OLED on.
void oled_power_off();

// write character at given position.
void oled_write_character(uint8_t character, uint8_t column, uint8_t row);

// sets a user defined character in the displays CGRAM.
// 8 user definable characters (char_n 1 to 8).
// characters are 5x8 (7 + cursor row).
// patterns pointer must point to 8 uint8_t rows.
void oled_set_character(uint8_t char_n, uint8_t const * const patterns);


// Writes an operation (display clear etc.). Checks the busy flag first.
void oled_write_cmd(uint8_t command);

// Writes an operation (display clear etc.).  Optionally checks the busy flag first.
void oled_write_cmd_busy(uint8_t command, bool wait_for_bf);

// Writes the given data to DDRAM or CGRAM.
void oled_write_data(uint8_t data);

// Set the x and y coordinates for graphics.  Top left is 1,1.
// note:  the cy co-ordinate is multiple of 8 pixels.
// eg. cy:1  y=1, 
//     cy:2  y=9.
void oled_set_coordinates(uint8_t x, uint8_t cy);

// write pixels at the given x and y co-ordinates.
// note:  the cy co-ordinate is multiple of 8 pixels.
void oled_write_pixels_at(uint8_t x, uint8_t cy, uint8_t pixels);

#endif /* CGOLED_H_ */
