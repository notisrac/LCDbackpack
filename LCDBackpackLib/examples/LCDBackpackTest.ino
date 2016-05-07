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
