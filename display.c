/*
 *	(c) 2015 László TÓTH
 *	(c) 2020 Stuart Hunter
 *
 *	TODO:
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

#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>

#include "display.h"
#include "oledimg.h"

#ifdef __arm__

// clang-format off
// retain this include order
#include "ArduiPi_OLED_lib.h"
#include "Adafruit_GFX.h"
#include "ArduiPi_OLED.h"
// clang-format on

#define SLEEP_TIME (25000 / 25)

ArduiPi_OLED display;
int sleep_divisor = 1;
int oledType = OLED_SH1106_I2C_128x64;


#endif

int maxCharacter(void) { return 21; } // review when we get scroll working

int maxLine(void) { return 8; }

int maxXPixel(void) { return 128; }

int maxYPixel(void) { return 64; }

#ifdef __arm__

void bigChar(uint8_t cc, int x, int len, int w, int h, const uint8_t font[]) {
  
  // need fix for space, and minus sign
  int start = (cc-48)*len;
  uint8_t dest[len];
  memcpy(dest, font+start, sizeof dest);
  display.drawBitmap(x, 1, dest, w, h, WHITE);

}

void resetDisplay(int fontSize) {
  display.clearDisplay(); // clears the screen  buffer
  display.setTextSize(fontSize);
  display.setTextColor(WHITE);
  display.setTextWrap(false);
  display.display(); // display it (clear display)
}

int initDisplay(void) {
  if (!display.init(OLED_I2C_RESET, oledType)) {
    return EXIT_FAILURE;
  }

  display.begin();
  resetDisplay(1);
  return 0;

}

void closeDisplay(void) {
  display.clearDisplay();
  display.close();
  return;
}

void splashScreen(void) {
  display.clearDisplay();
  display.drawBitmap(0, 0, splash, 128, 64, WHITE);
  display.display();
  delay(5000);
  display.clearDisplay();
  display.display();
}

void drawTimeBlink(uint8_t cc) {
  int x = 2+(2*25);
  if (32 == cc)
    display.fillRect(58, 0, 12, display.height()-16, BLACK);
  else
    bigChar(cc, x, LCD25X44_LEN, 25, 44, lcd25x44);
}

void drawTimeText(char *buff) {
  display.fillRect(0, 0, display.width(), display.height()-16, BLACK);
  // digit walk and "blit"
  int x = 2;
  for (size_t i = 0; i < strlen(buff); i++){
    bigChar(buff[i], x, LCD25X44_LEN, 25, 44, lcd25x44);
    x += 25;
  }
}

void drawHorizontalBargraph(int x, int y, int w, int h, int percent) {

  if (x == -1) {
    x = 0;
    w = display.width() - 2; // is a box so -2 for sides!!!
  }

  if (y == -1) {
    y = display.height() - h;
  }

  display.fillRect(x, y, (int16_t)w, h, 0);
  display.drawHorizontalBargraph(x, y, (int16_t)w, h, 1, (uint16_t)percent);

  return;
}

void refreshDisplay(void) { display.display(); }

void putText(int x, int y, char *buff) {
  display.setTextSize(1);
  display.fillRect(x, y, (int16_t)strlen(buff) * CHAR_WIDTH, CHAR_HEIGHT, 0);
  display.setCursor(x, y);
  display.print(buff);
}

void clearLine(int y) { display.fillRect(0, y, maxXPixel(), CHAR_HEIGHT, 0); }

void putTextToCenter(int y, char *buff) {
  int tlen = strlen(buff);
  int px = maxCharacter() < tlen ? 0 : (maxXPixel() - (tlen * CHAR_WIDTH)) / 2;
  clearLine(y);
  putText(px, y, buff);
}

void drawRectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
  display.drawRect(x, y, w, h, color);
}

// need scroller - independant threaded

/*

char message[] = "The quick Brown fox jumped Over the Lazy Dog";
int x, minX;

x = display.width();
minX = -CHAR_WIDTH * 2 * strlen(message); 

...

display.setCursor(x,line);
display.print(message);
if (x < minX)
  x = display.width();



*/

#endif
