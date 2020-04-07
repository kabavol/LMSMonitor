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

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#include "visdata.h"

#define CHAR_WIDTH 6
#define CHAR_HEIGHT 8

#define MAX_LINES 8
#define MAXSCROLL_DATA 255
#define MAX_BRIGHTNESS 220
#define NIGHT_BRIGHTNESS 100

#define XPOS 0
#define YPOS 1
#define DELTAY 2
#define DELTAX 3

#define NUMNOTES 10
#define NAGNOTE_HEIGHT 16
#define NAGNOTE_WIDTH  16

typedef enum PageMode {
    DETAILS,
    CLOCK,
    VISUALIZER
} PageMode;

typedef struct MonitorAttrs {
    int8_t oledAddrL;
    int8_t oledAddrR;
    int  vizHeight;
    int  vizWidth;
    bool allInOne;
    int  sleepTime;
    bool astral;
    bool showTemp;
    bool downmix;
    bool nagDone;
    bool visualize;
    bool meterMode;
    bool clock;
    bool extended;
    bool remaining;
    bool splash;
    bool refreshLMS;
    bool refreshClock;
    int  lastVolume;
    char lastBits[16];
    char lastTime[6];
    char lastTemp[10]; // should be a double
} MonitorAttrs;

typedef struct Scroller {
    pthread_mutex_t scrollox;
    bool initialized;
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
    void *(*scrollMe)(void *input);
} sme;

void printOledSetup(void);
void printOledTypes(void);
bool setOledType(int ot);
bool setOledAddress(int8_t oa);

double deg2Rad(double angDeg);
double rad2Deg(double angRad);

void splashScreen(void);
void displayBrightness(int bright);

void scrollerPause(void);
void *scrollLine(void *input); // threadable
void scrollerInit(void);
void clearScrollable(int line);
bool putScrollable(int y, char *buff);
void scrollerFinalize(void);
bool activeScroller(void);
void setScrollActive(int line, bool active);

void resetDisplay(int fontSize);
int initDisplay(void);
void closeDisplay(void);

// we'll support 2up and dual displays
// 2 up for now
//void vumeter2upl(void);
void stereoVU(struct vissy_meter_t *vissy_meter, char *downmix);
void stereoSpectrum(struct vissy_meter_t *vissy_meter, char *downmix);
void stereoPeakH(struct vissy_meter_t *vissy_meter, char *downmix);

// audio attributes
void putVolume(bool v, char *buff);
void putAudio(int a, char *buff);

void putText(int x, int y, char *buff);
void putTextCenterColor(int y, char *buff, uint16_t color);
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

void setSnapOn(void);
void setSnapOff(void);
void shotAndDisplay(void);

void nagSaverSetup(void);
void nagSaverNotes(void);

bool scrollerActive(void);

#endif
