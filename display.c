/*
 *	(c) 2015 László TÓTH
 *
 *	Todo:
 *
 *	This program is free software: you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation, either version 3 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	See <http://www.gnu.org/licenses/> to get a copy of the GNU General
 *	Public License.
 *
 */

#include "mixermon.h"

#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "display.h"

#ifdef __arm__

#include "ArduiPi_OLED_lib.h"
#include "Adafruit_GFX.h"
#include "ArduiPi_OLED.h"

#define SLEEP_TIME	(25000/25)

ArduiPi_OLED 		display;
int sleep_divisor	= 1 ;
int oledType		= OLED_SH1106_I2C_128x64;

#endif

int  maxCharacter(void) { return  21; }

int  maxLine(void)		{ return   8; }

int  maxXPixel(void)	{ return 128; }

int  maxYPixel(void)	{ return  64; }

#ifdef __arm__
/**********************************************************************
*
**********************************************************************/
int initDisplay(void) {
	if ( !display.init(OLED_I2C_RESET,oledType) ) {
		return EXIT_FAILURE;
	}

	display.begin();

	display.clearDisplay();			// clears the screen  buffer
	display.setTextSize(1);
	display.setTextColor(WHITE);
	display.display();				// display it (clear display)

	return 0;
}

//********************************************************************
void closeDisplay(void) {
	display.clearDisplay();

	// Free PI GPIO ports
	display.close();

	return;
}

//********************************************************************
void drawHorizontalBargraph(int x, int y, int w, int h, int percent) {
	if (x == -1) {
		x = 0;
		w = display.width();
	}

	if (y == -1) {
		y = display.height() - h;
	}

	display.fillRect(				x, y, (int16_t) w, h, 0);
	display.drawHorizontalBargraph(	x, y, (int16_t) w, h, 1, (uint16_t) percent);

	return;
}

void refreshDisplay(void) {
	display.display();
}


//********************************************************************
void putText(int x, int y, char *buff) {
	display.setTextSize(1);
	display.fillRect( x, y, (int16_t) strlen(buff) * CHAR_WIDTH, CHAR_HEIGHT, 0);
	display.setCursor(x, y);
	display.print(buff);
}

//********************************************************************
void clearLine(int y) {
	display.fillRect( 0, y, maxXPixel(), CHAR_HEIGHT, 0);
}

//********************************************************************
void putTextToCenter(int y, char *buff) {
	int tlen = strlen(buff);
	int px = maxCharacter() < tlen ? 0 : (maxXPixel() - (tlen * CHAR_WIDTH)) / 2;
	clearLine(y);
	putText(px, y, buff);
}

#endif
