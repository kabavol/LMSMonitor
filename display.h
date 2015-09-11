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

#ifndef DISPLAY_H
#define DISPLAY_H 1

#define CHAR_WIDTH  6
#define CHAR_HEIGHT 8

int  initDisplay(void);
void closeDisplay(void);
void drawHorizontalBargraph(int x, int y, int w, int h, int percent);
void putText(int x, int y, char *buff);
void putTextToCenter(int y, char *buff);
void clearLine(int y);
void refreshDisplay(void);
int  maxCharacter(void);
int  maxLine(void);
int  maxXPixel(void);
int  maxYPixel(void);

#endif
