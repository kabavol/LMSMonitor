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

#ifndef PROGMEM
#define PROGMEM
#endif

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include "gfxfont.h"
//#include "../font/NotoSansRegular5lms.h"

#include "visdata.h"

#define MAX_LINES 8

#define MAXSCROLL_DATA 255
#define MAX_BRIGHTNESS 200
#define NIGHT_BRIGHTNESS 90

#define XPOS 0
#define YPOS 1
#define DELTAY 2
#define DELTAX 3

#define NUMNOTES 10
#define NAGNOTE_HEIGHT 16
#define NAGNOTE_WIDTH  16

#define SCROLL_MODE_CYLON  0 // cylon sweep with pause
#define SCROLL_MODE_INFSIN 1 // infinity scroll left (sinister)
#define SCROLL_MODE_INFDEX 2 // infinity scroll right (dexter)
#define SCROLL_MODE_RANDOM 3 // randomize the above
#define SCROLL_MODE_MAX 4

#define SHUFFLE_BUCKET 0
#define REPEAT_BUCKET  1
#define MAX_FREQUENCY_BINS 12
// frequency cap decay
#define CAPS_DECAY 0.075

static const char * scrollerMode[] = {
    "Cylon (Default)",
    "Infinity (Sinister)",
    "Infinity (Dexter)",
    "Randomized"
};

typedef enum PageMode {
    DETAILS,
    CLOCK,
    VISUALIZER,
    ALLINONE
} PageMode;

typedef struct audio_t {
    double samplerate; 
    int8_t samplesize; 
    int8_t volume;
    int8_t audioIcon;
    int8_t repeat;
    int8_t shuffle;
} audio_t;

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
    bool refreshViz;
    int  lastVolume;
    uint8_t lastModes[2]; // shuffle[0] + repeat[1]
    char lastBits[16];
    char lastTime[6];
    char lastTemp[10]; // should be a double
    char lastLoad[10]; // should be a double
    pthread_mutex_t update;
} MonitorAttrs;

typedef struct Scroller {
    bool active;
    bool initialized;
    int textPix;
    int line;
    int xpos;
    int ypos;
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
    int xPos;
    int yPos;
    bool largeFont;
} DrawTime; // generic font header!

void printFontMetrics(void);

void setInitRefresh(void);

void printOledSetup(void);
void printOledTypes(void);
bool setOledType(int ot);
bool setOledAddress(int8_t oa);

void setScrollMode(int sm);
void printScrollerMode(void);

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
void setScrollActive(int line, bool active);
bool activeScroller(void);

void resetDisplay(int fontSize);
int initDisplay(void);
void closeDisplay(void);
void softClear(void);

void stereoVU(struct vissy_meter_t *vissy_meter, char *downmix);
void stereoSpectrum(struct vissy_meter_t *vissy_meter, char *downmix);
void ovoidSpectrum(struct vissy_meter_t *vissy_meter, char *downmix);
void mirrorSpectrum(struct vissy_meter_t *vissy_meter, char *downmix);
void reflectSpectrum(struct vissy_meter_t *vissy_meter, char *downmix);
void stereoPeakH(struct vissy_meter_t *vissy_meter, char *downmix);

// audio attributes
void putVolume(bool v, char *buff);
void putAudio(audio_t audio, char *buff, bool full=true);

void putText(int x, int y, char *buff);
void putTextCenterColor(int y, char *buff, uint16_t color);
void putTextToCenter(int y, char *buff);

void clearLine(int y);
void clearDisplay();

void drawTimeBlink(uint8_t cc, DrawTime *dt);
void drawTimeText(char *buff, char *last, DrawTime *dt);

void refreshDisplay(void);
void refreshDisplayScroller(void);

int maxCharacter(void);
int maxLine(void);
int maxXPixel(void);
int maxYPixel(void);
uint16_t charWidth(void);
uint16_t charHeight(void);

void drawHorizontalBargraph(int x, int y, int w, int h, int percent);
void drawRectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

void setSnapOn(void);
void setSnapOff(void);
void shotAndDisplay(void);

void nagSaverSetup(void);
void nagSaverNotes(void);

#endif
