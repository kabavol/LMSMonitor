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

#ifndef DISPLAY_H
#define DISPLAY_H 1

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#include "visdata.h"

#define CHAR_WIDTH 6
#define CHAR_HEIGHT 8

#define MAX_LINES 8
#define MAXSCROLL_DATA 255

 typedef struct Scroller {
    char *text;
    pthread_t scrollThread;
    int textPix;
    int line;
    int xpos;
    int ypos;
    bool nystagma;
    int lolimit;
    int hilimit;
    bool forward;
    bool active;
    bool pause;
    void* (*scrollMe)(void *input);
} sme;

void splashScreen(void);
 
void scrollerPause(void);
void* scrollLine(void *input); // threadable
void scrollerInit(void); 
void putScrollable(int y, char *buff);
void scrollerFinalize(void);

void resetDisplay(int fontSize);
int initDisplay(void);
void closeDisplay(void);

// we'll support 2up and dual displays
void vumeter2upl(void);
void stereoVU(struct vissy_meter_t *vissy_meter);

// audio attributes
void putVolume(bool v, char *buff);
void putAudio(int a, char *buff);

void putText(int x, int y, char *buff);
void putTextToCenter(int y, char *buff);
void drawTimeBlink(uint8_t cc);
void drawTimeText(char *buff);
void drawTimeText2(char *buff, char *last);
void clearLine(int y);
void clearDisplay();

void refreshDisplay(void);
void refreshDisplayScroller(void);

int maxCharacter(void);
int maxLine(void);
int maxXPixel(void);
int maxYPixel(void);

void drawHorizontalBargraph(int x, int y, int w, int h, int percent);
void drawRectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

// experimental
void testFont(int x, int y, char *buff);

#endif
