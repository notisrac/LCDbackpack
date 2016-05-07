/*

LCDBackpackLib - Arduino library for driving the LCD display on the LCDBackpack


Copyright (C) 2016 noti

GNU GPLv3 license

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.


Contact: notix@freemail.hu


Acknowledgements:

  raron's ShiftRegLCD123 - this lib is based on raron's work:
    http://code.google.com/p/shiftreglcd123/
  A proper initialization routine for HD44780 compatible LCD's:
    http://web.alfredstate.edu/weimandn/lcd/lcd_initialization/lcd_initialization_index.html


For reference / schematics:

Project homepage: https://github.com/notisrac/LCDbackpack


CONNECTION DESCRIPTIONS

  Requires 3 pins from the Arduino.

SR output:  Wiring:
  Bit  #0   - N/C - not connected.
  Bit  #1   - connects to RS (Register Select) on the LCD
  Bit  #2   - N/C or LCD backlight. Do not connect directly!
              Use a driver / transistor!
  Bit  #3   - connects to LCD Enable input.
  Bits #4-7 - connects to LCD data inputs D4 - D7.

  LCD R/!W-pin hardwired to LOW (only writing to LCD).


USAGE:

  1: Make an LCD object, set arduino output pins and LCD wiring scheme:

        ShiftRegLCD123 LCDobject( Datapin , Clockpin, Latchpin )

    where:
      Datapin : Arduino pin to shiftregister serial data input. D11
      Clockpin: Arduino pin to shiftregister clock input. D13
      Latchpin: Arduino pin to shiftregister latch/strobe/register clock input. D10

  2: Initialize the LCD by calling begin() function with LCD size and font:

        LCDobject.begin( cols, lines [, font] )

    where:
      cols    : Nr. of columns in the LCD
      lines   : Nr. of "logical lines" in the LCD (not neccesarily physical)
      font    : 0 = small (default), 1 = large font for some 1-line LCD's only.


History
2016.04.17 noti - first version

*/

#include "LCDBackpackLib.h"
#include <stdio.h>
#include <inttypes.h>

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif


LCDBackpackLib::LCDBackpackLib(uint8_t srdata, uint8_t srclock, uint8_t srlatch)
{
	init(srdata, srclock, srlatch);
}


void LCDBackpackLib::init(uint8_t srdata, uint8_t srclock, uint8_t srlatch)
{
	_srdata_pin = srdata;
	_srclock_pin = srclock;
	_srlatch_pin = srlatch;
	_backlight = 0;       // Defaults to backlight off.

	pinMode(_srclock_pin, OUTPUT);
	pinMode(_srdata_pin, OUTPUT);
	pinMode(_srlatch_pin, OUTPUT);

	//digitalWrite(_srlatch_pin, LOW);
	shiftOut(_srdata_pin, _srclock_pin, MSBFIRST, 0x00);
	digitalWrite(_srlatch_pin, LOW);
}

void LCDBackpackLib::begin(uint8_t cols, uint8_t lines)
{
	begin(cols, lines, 0);
}

void LCDBackpackLib::begin(uint8_t cols, uint8_t lines, uint8_t font)
{
	_numlines = lines;
	_cols = cols;    // Only used for identifying 16x4 LCD, nothing else.

	if (lines > 1)
	{
		_displayfunction = LCD_4BITMODE | LCD_2LINE;
	}
	else
	{
		_displayfunction = LCD_4BITMODE | LCD_1LINE;
	}


	// For some 1-line displays you can select a 10 pixel high font
	if (font != 0 && lines == 1)
	{
		_displayfunction |= LCD_5x10DOTS;
	}
	else
	{
		_displayfunction |= LCD_5x8DOTS;
	}

	// SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
	// according to datasheet, we need at least 40ms after power rises above 2.7V
	// before sending commands. Arduino can turn on way before 4.5V so we'll wait 
	// more.
	// Also check this -excellent- guide on HD44780-based LCD initializations:
	// http://web.alfredstate.edu/weimandn/lcd/lcd_initialization/lcd_initialization_index.html
	// (From which this initialization and comments are taken from)
	// ---------------------------------------------------------------------------

	// Step 1.   Power on, then delay > 100 ms
	//delay(200);
	for (int i = 0; i < 200; ++i)
	{
		delayMicroseconds(1000);  // Waits 200 ms just in case.
	}

	// Step 2.   Instruction 0011b (3h), then delay > 4.1 ms 
	send(LCD_FUNCTIONSET | LCD_8BITMODE, MODE_INIT);
	delayMicroseconds(4500);  // wait more than 4.1ms

	// Step 3.   Instruction 0011b (3h), then delay > 100 us
	//   NOTE: send()/command() and init4bits() functions adds 60 us delay
	send(LCD_FUNCTIONSET | LCD_8BITMODE, MODE_INIT);
	delayMicroseconds(40);

	// Step 4.   Instruction 0011b (3h), then delay > 100 us 
	send(LCD_FUNCTIONSET | LCD_8BITMODE, MODE_INIT);
	delayMicroseconds(40);

	// Step 5.   Instruction 0010b (2h), then delay > 100 us (speculation)
	//   This is where display is set to 4-bit interface
	send(LCD_FUNCTIONSET | LCD_4BITMODE, MODE_INIT);
	delayMicroseconds(40);

	// Step 6.   Instruction 0010b (2h), then 1000b (8h), then delay > 53 us or check BF 
	//   Set nr. of logical lines (not neccesarily physical lines) and font size
	//   (usually 2 lines and small font)
	command(LCD_FUNCTIONSET | _displayfunction);
	//delayMicroseconds(60);

	// Step 7.   Instruction 0000b (0h), then 1000b (8h) then delay > 53 us or check BF
	command(LCD_DISPLAYCONTROL);
	//delayMicroseconds(60);

	// Step 8.   Instruction 0000b (0h), then 0001b (1h) then delay > 3 ms or check BF 
	//   This is a Clear display instruction and takes a lot of time
	command(LCD_CLEARDISPLAY);
	delayMicroseconds(3000);

	// Step 9.   Instruction 0000b (0h), then 0110b (6h), then delay > 53 us or check BF 
	//   Initialize to default text direction (for romance languages)
	_displaymode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
	command(/*LCD_DISPLAYCONTROL*/ LCD_ENTRYMODESET | _displaymode);
	//delayMicroseconds(60);

	// Step 10. Not really a step, but initialization has ended. Except display is turned off.

	// Step 11.   Instruction 0000b (0h), then 1100b (0Ch), then delay > 53 us or check BF
	//   Turn display on, no cursor, no cursor blinking.
	_displaycontrol = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
	//delayMicroseconds(60);
}

// Commands used by the other functions to control the LCD

void LCDBackpackLib::command(uint8_t value)
{
	send(value, MODE_COMMAND);
}

#if defined(ARDUINO) && ARDUINO >= 100
size_t LCDBackpackLib::write(uint8_t value)
#else
void LCDBackpackLib::write(uint8_t value)
#endif
{
	send(value, MODE_WRITE);

	return 1;
}

// For sending data via the shiftregister

void LCDBackpackLib::send(uint8_t value, uint8_t mode)
{
	// for normal operation a B0000 needs to be sent first, then the actual instruction
	// for init, ony one nibble needs to be sent
	for (size_t nibbleNumber = 0; nibbleNumber < 2; nibbleNumber++)
	{
		//                  DDDDEBR
		uint8_t dataBits = B00000000;
		// skip the  lower nibble when initializing
		if (nibbleNumber > 0 && mode == MODE_INIT)
		{
			continue;
		}
		if (mode == MODE_INIT)
		{
			// set the data
			dataBits |= value; //(value << 4);
		}
		else // if (0 < nibbleNumber && mode != MODE_INIT)
		{
			// the first 4bits contains the first instruction, the second 4 the second instruction
			if (0 == nibbleNumber)
			{
				// get the first nibble only (= remove the second 4bits)
				dataBits |= ((value >> 4) << 4);
			}
			else
			{
				// get the second 4bits
				dataBits |= (value << 4);
			}
			// set the RS bit: LOW = command,  HIGH = character
			dataBits |= mode ? RS_BIT : 0;
		}
		// set the backlight bit
		dataBits |= _backlight ? BACKLIGHT_BIT : 0;
		// set the enable bit on
		dataBits |= EN_BIT;
		// send
		shiftOutBits(dataBits);
		// enable pulse must be >450ns  
		delayMicroseconds(1);

		// set the enable bit off
		dataBits &= (uint8_t)~EN_BIT;
		// send
		shiftOutBits(dataBits);
	}

	// commands need > 37us to settle (for normal LCD with clock = 270 kHz)
	// Since LCD's may be slow (190 kHz clock = 53 us), we'll wait 60 us
	delayMicroseconds(60);
}

void LCDBackpackLib::shiftOutBits(uint8_t value)
{
	digitalWrite(_srlatch_pin, LOW);
	shiftOut(_srdata_pin, _srclock_pin, MSBFIRST, value);
	digitalWrite(_srlatch_pin, HIGH);
}


// PUBLIC METHODS
// high level commands, for the user
// ---------------------------------------------------------------------------

// Backlight On / Off control
void LCDBackpackLib::backlightOn()
{
	_backlight = 1;
	// Just to force a write to the shift register
	command(LCD_DISPLAYCONTROL | LCD_DISPLAYON);
}

void LCDBackpackLib::backlightOff()
{
	_backlight = 0;
	// Just to force a write to the shift register
	command(LCD_DISPLAYCONTROL | LCD_DISPLAYON);
}

void LCDBackpackLib::clear()
{
	command(LCD_CLEARDISPLAY);  // clear display, set cursor position to zero
	delayMicroseconds(3000);    // this command takes a long time!
}

void LCDBackpackLib::home()
{
	command(LCD_RETURNHOME);    // set cursor position to zero
	delayMicroseconds(3000);    // this command takes a long time!
}


void LCDBackpackLib::setCursor(uint8_t col, uint8_t row)
{
	const uint8_t row_offsetsDef[] = { 0x00, 0x40, 0x14, 0x54 }; // For regular LCDs
	const uint8_t row_offsetsLarge[] = { 0x00, 0x40, 0x10, 0x50 }; // For 16x4 LCDs

	if (row >= _numlines) row = _numlines - 1;    // rows start at 0

												  // 16x4 LCDs have special memory map layout
												  // ----------------------------------------
	if (_cols == 16 && _numlines == 4)
		command(LCD_SETDDRAMADDR | (col + row_offsetsLarge[row]));
	else
		command(LCD_SETDDRAMADDR | (col + row_offsetsDef[row]));
}


// Turn the display on/off (quickly)
void LCDBackpackLib::noDisplay() {
	_displaycontrol &= ~LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LCDBackpackLib::display() {
	_displaycontrol |= LCD_DISPLAYON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turns the underline cursor on/off
void LCDBackpackLib::noCursor() {
	_displaycontrol &= ~LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LCDBackpackLib::cursor() {
	_displaycontrol |= LCD_CURSORON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// Turn on and off the blinking cursor
void LCDBackpackLib::noBlink() {
	_displaycontrol &= ~LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}
void LCDBackpackLib::blink() {
	_displaycontrol |= LCD_BLINKON;
	command(LCD_DISPLAYCONTROL | _displaycontrol);
}

// These commands scroll the display without changing the RAM
void LCDBackpackLib::scrollDisplayLeft(void) {
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}
void LCDBackpackLib::scrollDisplayRight(void) {
	command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

// This is for text that flows Left to Right
void LCDBackpackLib::leftToRight(void) {
	_displaymode |= LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This is for text that flows Right to Left
void LCDBackpackLib::rightToLeft(void) {
	_displaymode &= ~LCD_ENTRYLEFT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'right justify' text from the cursor
void LCDBackpackLib::autoscroll(void) {
	_displaymode |= LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// This will 'left justify' text from the cursor
void LCDBackpackLib::noAutoscroll(void) {
	_displaymode &= ~LCD_ENTRYSHIFTINCREMENT;
	command(LCD_ENTRYMODESET | _displaymode);
}

// Allows us to fill the first 8 CGRAM locations with custom characters
void LCDBackpackLib::createChar(uint8_t location, uint8_t charmap[]) {
	location &= 0x7;              // we only have 8 locations 0-7
	command(LCD_SETCGRAMADDR | location << 3);
	for (int i = 0; i<8; i++) {
		write((uint8_t)charmap[i]);
	}
	command(LCD_SETDDRAMADDR);    // Reset display to display text (from pos. 0)
}

