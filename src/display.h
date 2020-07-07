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

#include "gfxfont.h"
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
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

typedef enum PageMode { DETAILS, CLOCK, VISUALIZER, ALLINONE } PageMode;

typedef struct point_t {
    int x;
    int y;
} point_t;

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
    int vizHeight;
    int vizWidth;
    bool allInOne;
    bool tapeUX;   // cassette easter egg
    bool sl1200UX; // Technics SL-1200 easter egg
    int sleepTime;
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
    int lastVolume;
    int clockFont;
    bool flipDisplay;     // display mounted upside down
    uint8_t i2cBus;       // number of I2C bus
    uint8_t oledRST;      // IIC/SPI reset GPIO
    uint8_t spiDC;        // SPI DC
    uint8_t spiCS;        // SPI CS - 0: CS0, 1: CS1
    uint16_t spiSpeed;    // SPI speed - one of supported values - review
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

void compactCassette(void);
void cassetteHub(int xpos, int frame, int mxframe, int direction);

void toneArm(double pct, bool init);
void technicsSL1200(bool blank);
void vinylEffects(int xpos, int lpos, int frame, int maxf);

void printFontMetrics(void);

void flipDisplay(struct MonitorAttrs dopts);

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
void displayBrightness(int bright, bool flip = false);

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
int initDisplay(struct MonitorAttrs dopts);
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
void putTapeType(audio_t audio);
void putSL1200Btn(audio_t audio);

void putIFDetail(int icon, int xpos, int ypos, char *host);

void putTextMaxWidth(int x, int y, int w, char *buff);
void putText(int x, int y, char *buff);
void putTextCenterColor(int y, char *buff, uint16_t color);
void putTextToCenter(int y, char *buff);

// using tom thumb font to squeeze a little more real-estate
void putTinyTextMaxWidth(int x, int y, int w, char *buff);
void putTinyTextMaxWidthP(int x, int y, int w, char *buff);
void putTinyTextMultiMaxWidth(int x, int y, int w, int lines, char *buff);
void putTinyText(int x, int y, char *buff);
void putTinyTextCenterColor(int y, char *buff, uint16_t color);
void putTinyTextToCenter(int y, char *buff);

void clearLine(int y);
void clearDisplay();

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

void drawHorizontalBargraph(int x, int y, int w, int h, int percent);
void drawHorizontalCheckerBar(int x, int y, int w, int h, int percent);
void drawRectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

void setSnapOn(void);
void setSnapOff(void);
void shotAndDisplay(void);

void nagSaverSetup(void);
void nagSaverNotes(void);

#endif
