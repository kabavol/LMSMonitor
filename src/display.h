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

enum inching { IW_WIDTH, IW_HEIGHT };
enum honeymethod { IM_SHRINK, IM_GROW };
enum inchdir {
    ID_GLEFT,
    ID_GRIGHT,
    ID_GDOWN,
    ID_GUP,
    ID_SLEFT,
    ID_SRIGHT,
    ID_SDOWN,
    ID_SUP
};

typedef struct point_t {
    int16_t x;
    int16_t y;
} point_t;

typedef struct limits_t {
    int16_t min;
    int16_t max;
} limits_t;

typedef struct dimention_t {
    point_t tlpos;
    int16_t w;
    int16_t h;
    int16_t radius;
} dimention_t;

// drawRoundRect(int16_t x0, int16_t y0, int16_t w, int16_t h, int16_t radius, uint16_t color)
typedef struct inchingball_t {
    dimention_t dimention;
    enum inching direction; // modifying width/height
    enum honeymethod hm;    // -1/+1 drives shrink/grow
    enum inchdir sliding;   // modifying direction
} inchingball_t;

typedef struct plays_t {
    int adj;
    enum inching direction; // modifying width/height
    enum honeymethod hm;    // -1/+1 drives shrink/grow
    enum inchdir sliding;   // modifying direction
} plays_t;

typedef struct inching_t {
    int drinkme; // active shrink/grow object
    int currseq; // current play
    int playcnt;
    point_t pin;
    limits_t limitw;
    limits_t limith;
    limits_t limitx;
    limits_t limity;
    inchingball_t incher[3];
    plays_t playseq[9];
} inching_t;

typedef struct paddle_t {
    point_t pos;
    int16_t w;
    int16_t h;
} paddle_t; 

typedef struct pongplay_t {
    point_t left;
    point_t right;
    point_t ball;
} pongplay_t;
typedef struct pongem_t {
    int currseq; // current play
    int playcnt;
    point_t fieldpos;
    int16_t fieldw;
    int16_t fieldh;
    paddle_t left;
    paddle_t right;
    point_t ball;
    pongplay_t playseq[20];
} pongem_t;

typedef struct audio_t {
    double samplerate;
    int8_t samplesize;
    int8_t volume;
    int8_t audioIcon;
    int8_t repeat;
    int8_t shuffle;
} audio_t;

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

void compactCassette(void);
void cassetteEffects(int xpos, int frame, int mxframe, int direction);

void toneArm(double pct, bool init);
void technicsSL1200(bool blank);
void vinylEffects(int xpos, int lpos, int frame, int mxframe);

void reelToReel(bool blank);
void reelEffects(int xpos, int ypos, int frame, int mxframe, int direction);

void vcrPlayer(bool blank);
void vcrEffects(int xpos, int ypos, int frame, int mxframe);

void radio50(bool blank);
void radioEffects(int xpos, int ypos, int frame, int mxframe);

void TVTime(bool blank);
void PCTime(bool blank);
void pong(void);

void printFontMetrics(void);

void flipDisplay(struct MonitorAttrs dopts);

void setInitRefresh(void);

void printOledSetup(void);
void printOledTypes(void);
bool setOledType(int ot);
bool setOledAddress(int8_t oa, int LR = 0);

void setScrollMode(int sm);
void printScrollerMode(void);

double deg2Rad(double angDeg);
double rad2Deg(double angRad);

void splashScreen(void);
void displayBrightness(int bright, bool flip = false);

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
void putReelToReel(audio_t audio);
void putVcr(audio_t audio);
void putRadio(audio_t audio);
void putTVTime(audio_t audio);
void putPCTime(audio_t audio);

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
void fillRectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);

void setSnapOn(void);
void setSnapOff(void);
void shotAndDisplay(void);

void nagSaverSetup(void);
void nagSaverNotes(void);

inching_t *initInching(const point_t pin, const limits_t lw, const limits_t lh,
                       const limits_t lx, const limits_t ly);
void animateInching(inching_t *b);

#endif
