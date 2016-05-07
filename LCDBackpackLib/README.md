# LCDBackpackLib
Since the LCDBackpack uses a different schematic, than the other LCD libraries, you need to use this library to be able to make the LCDBackpack work.


##Acknowledgements
raron's ShiftRegLCD123 - this lib is based on raron's work:
  http://code.google.com/p/shiftreglcd123/

A proper initialization routine for HD44780 compatible LCD's:
  http://web.alfredstate.edu/weimandn/lcd/lcd_initialization/lcd_initialization_index.html

##Schematic
See the project page: https://github.com/notisrac/LCDbackpack

##Connections

  Requires 3 pins from the Arduino.

SR output - Wiring:
*  Bit  #0   - N/C - not connected.
*  Bit  #1   - connects to RS (Register Select) on the LCD
*  Bit  #2   - N/C or LCD backlight. Do not connect directly! Use a driver / transistor!
*  Bit  #3   - connects to LCD Enable input.
*  Bits #4-7 - connects to LCD data inputs D4 - D7.
*  LCD R/!W-pin hardwired to LOW (only writing to LCD).


##Usage

```c
#include <LCDBackpackLib.h>

const byte dataPin = 11;    // SR Data from Arduino pin 10
const byte clockPin = 13;    // SR Clock from Arduino pin 11
const byte latchPin = 10;   // LCD enable from Arduino pin 12

// Instantiate an LCD object
LCDBackpackLib lcdbp(dataPin, clockPin, latchPin);

void setup()
{
	// initialize LCD and set display size
	// LCD size 20 columns x 2 lines, small (normal) font
	lcdbp.begin(20, 2);

	// Turn on backlight (if used)
	lcdbp.backlightOn();

	// Print a message to the LCD.
	lcdbp.print("HELLO, WORLD!");

	// move to next line
	lcdbp.setCursor(0, 1);
	lcdbp.print("LCDBackpackLib test"); 
}

void loop()
{
}
```
