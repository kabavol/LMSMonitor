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
#define DISPLAY_H

#ifndef PROGMEM
#define PROGMEM
#endif

#include "climacell.h"
#include "gfxfont.h"
#include "lmsopts.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#include "visdata.h"
#include "eggs.h"

#define MAX_LINES 8

#define MAXSCROLL_DATA 255
#define MAX_BRIGHTNESS 200
#define NIGHT_BRIGHTNESS                                                       \
    88 // need to track down bit leak causing screen to flip/misalign

#define XPOS 0
#define YPOS 1
#define DELTAY 2
#define DELTAX 3

#define NUMNOTES 10
#define NAGNOTE_HEIGHT 16
#define NAGNOTE_WIDTH 16

#define SCROLL_MODE_CYLON 0  // cylon sweep with pause
#define SCROLL_MODE_INFSIN 1 // infinity scroll left (sinister)
#define SCROLL_MODE_INFDEX 2 // infinity scroll right (dexter)
#define SCROLL_MODE_RANDOM 3 // randomize the above
#define SCROLL_MODE_MAX 4

#define SHUFFLE_BUCKET 0
#define REPEAT_BUCKET 1
#define MAX_FREQUENCY_BINS 12
// frequency cap decay
#define CAPS_DECAY 0.075

#define A1SCROLLPOS 50
#define TXSCROLLPOS 8

static const char *scrollerMode[] = {"Cylon (Default)", "Infinity (Sinister)",
                                     "Infinity (Dexter)", "Randomized"};

typedef enum PageMode {
    DETAILS,
    CLOCK,
    VISUALIZER,
    ALLINONE,
    EGG,
    WEATHER
} PageMode;

typedef struct Scroller {
    bool active;
    bool initialized;
    int textPix;
    int line;
    point_t pos;
    bool nystagma;
    int lolimit;
    int hilimit;
    bool forward;
    int scrollMode;
    pthread_t scrollThread;
    pthread_mutex_t scrollox;
    void *(*scrollMe)(void *input);
    char *text;
} sme;

typedef struct DrawTime {
    int charWidth;
    int charHeight;
    int bufferLen;
    point_t pos;
    int font;
    bool fmt12;
} DrawTime; // generic font header!

typedef struct DrawVisualize {
    point_t pos;
    double wMeter;
    double hMeter;
    double rMeter;
    int iWidth;
    int iHeight;
    bool finesse;
    char downmix[5];
} DrawVisualize;

int elementLength(int szh, int szw);

void printFontMetrics(void);

void flipDisplay(struct MonitorAttrs dopts);

void setInitRefresh(void);

void printOledSetup(void);
void printOledTypes(void);
bool setOledType(int ot);
bool setOledAddress(int8_t oa, int LR = 0);

void setScrollMode(int sm);
void printScrollerMode(void);

void drawBitmap(int16_t x, int16_t y,
  uint8_t *bitmap, int16_t w, int16_t h, uint16_t color);

double deg2Rad(double angDeg);
double rad2Deg(double angRad);

void hazardSign(void);
void splashScreen(void);
void displayBrightness(int bright, bool flip=false);

void putWeatherTemp(int x, int y, climacell_t *cc);
void putWeatherIcon(int x, int y, climacell_t *cc);

void scrollerPause(void);
void *scrollLine(void *input); // threadable
void scrollerInit(void);
void clearScrollable(int line);
bool putScrollable(int y, char *buff);
void scrollerFinalize(void);
void setScrollActive(int line, bool active);
void setScrollPosition(int line, int ypos);
bool activeScroller(void);
bool isScrollerActive(int line);

void resetDisplay(int fontSize);
int restartDisplay(struct MonitorAttrs dopts);
int initDisplay(struct MonitorAttrs dopts, bool init=true);
void closeDisplay(void);
void softClear(void);

void stereoVU(struct vissy_meter_t *vissy_meter, struct DrawVisualize *layout);
void stereoSpectrum(struct vissy_meter_t *vissy_meter,
                    struct DrawVisualize *layout);
void ovoidSpectrum(struct vissy_meter_t *vissy_meter,
                   struct DrawVisualize *layout);
void mirrorSpectrum(struct vissy_meter_t *vissy_meter,
                    struct DrawVisualize *layout);
void reflectSpectrum(struct vissy_meter_t *vissy_meter,
                     struct DrawVisualize *layout);
void stereoPeakH(struct vissy_meter_t *vissy_meter,
                 struct DrawVisualize *layout);

// audio attributes
void putVolume(bool v, char *buff);
void putAudio(audio_t audio, char *buff, bool full = true);
int putWarning(char *msg, bool init);

void putIFDetail(int icon, int xpos, int ypos, char *host);

void putTextMaxWidth(int x, int y, int w, char *buff);
void putText(int x, int y, char *buff);
void putTextCenterColor(int y, char *buff, uint16_t color);
void putTextToCenter(int y, char *buff);
void putTextToRight(int y, int r, char *buff);

// using tom thumb font to squeeze a little more real-estate
void putTinyTextMaxWidth(int x, int y, int w, char *buff);
void putTinyTextMaxWidthP(int x, int y, int w, char *buff);
void putTinyTextMultiMaxWidth(int x, int y, int w, int lines, char *buff);
void putTinyText(int x, int y, char *buff);
void putTinyTextCenterColor(int y, char *buff, uint16_t color);
void putTinyTextToCenter(int y, char *buff);
void putTinyTextToRight(int y, int r, int w, char *buff);

void clearLine(int y);
void clearDisplay(void);

void drawTimeBlink(uint8_t cc, DrawTime *dt);
void drawTimeText(char *buff, char *last, DrawTime *dt);
void drawRemTimeText(char *buff, char *last, DrawTime *dt);

void refreshDisplay(void);
void refreshDisplayScroller(void);

int maxCharacter(void);
int maxLine(void);
int maxXPixel(void);
int maxYPixel(void);
uint16_t charWidth(void);
uint16_t charHeight(void);

// drawing primitives
void drawHorizontalBargraph(int x, int y, int w, int h, int percent);
void drawHorizontalCheckerBar(int x, int y, int w, int h, int percent);
void drawRectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void drawRoundRectangle(int16_t x0, int16_t y0, int16_t w, int16_t h,int16_t radius, uint16_t color);
void fillRectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void putPixel(int16_t x, int16_t y, uint16_t color);

void setSnapOn(void);
void setSnapOff(void);
void shotAndDisplay(void);

void nagSaverSetup(void);
void nagSaverNotes(void);

#endif
