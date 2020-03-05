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

#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>

#include "display.h"
#include "oledimg.h"
#include "libm6.h"

#define _USE_MATH_DEFINES
#define PI acos(-1.0000)

#ifdef __arm__

// clang-format off
// retain this include order
#include "ArduiPi_OLED_lib.h"
#include "Adafruit_GFX.h"
#include "ArduiPi_OLED.h"
// clang-format on

ArduiPi_OLED display;
int oledType = OLED_SH1106_I2C_128x64;

#endif

sme scroll[MAX_LINES];

int maxCharacter(void) { return 21; } // review when we get scroll working

int maxLine(void) { return MAX_LINES; }

int maxXPixel(void) { return 128; }

int maxYPixel(void) { return 64; }

#ifdef __arm__

void bigChar(uint8_t cc, int x, int len, int w, int h, const uint8_t font[]) {

    // need fix for space, and minus sign
    int start = (cc - 48) * len;
    uint8_t dest[len];
    memcpy(dest, font + start, sizeof dest);
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
    scrollerInit();
    return 0;
}

void closeDisplay(void) {
    scrollerFinalize();
    display.clearDisplay();
    display.close();
}

void clearDisplay() {
    display.clearDisplay();
    display.display();
}

void vumeter2upl(void) {
    display.clearDisplay();
    display.drawBitmap(0, 0, vu2up128x64, 128, 64, WHITE);
}

void splashScreen(void) {
    display.clearDisplay();
    display.drawBitmap(0, 0, splash, 128, 64, WHITE);
    display.display();
    delay(2000);
}

long long last_L = -1000;
long long last_R = -1000;

void stereoVU(struct vissy_meter_t *vissy_meter)
{

    // do no work if we don't need to
    if ((last_L = vissy_meter->rms_bar[0]) && (last_R == vissy_meter->rms_bar[1]))
        return;

    last_L = vissy_meter->rms_bar[0];
    last_R = vissy_meter->rms_bar[1];

    vumeter2upl();

    int hMeter = maxYPixel()-4;
    int rMeter = hMeter-6;
    int16_t wMeter = maxXPixel()/2;
    int16_t xpivot[2];
    xpivot[0] = maxXPixel()/4;
    xpivot[1] = xpivot[0] * 3;
    double rad = (180.00 / PI); // 180/pi

    // meter positions

    for (int channel = 0; channel < 2; channel++) {

        // meter value
	    double mv = (double)vissy_meter->rms_scale[channel] * (2 * 36.00) / 48.00;
	    mv -= 36.000; // zero adjust [-3dB->-20dB]

	    int16_t ax = (xpivot[channel] + (sin(mv/rad) * rMeter));
	    int16_t ay = (wMeter - (cos(mv/rad) * rMeter));

        // thick needle with definition
		display.drawLine(xpivot[channel]-2, wMeter, ax, ay, BLACK);
		display.drawLine(xpivot[channel]-1, wMeter, ax, ay, WHITE);
		display.drawLine(xpivot[channel], wMeter, ax, ay, WHITE);
		display.drawLine(xpivot[channel]+1, wMeter, ax, ay, WHITE);
		display.drawLine(xpivot[channel]+2, wMeter, ax, ay, BLACK);

    }

    // finesse
    display.fillRect(xpivot[0]-3, maxYPixel()-6, maxXPixel()/2, 6, BLACK);
    uint16_t r = 7;
    for (int channel = 0; channel < 2; channel++) {
        display.fillCircle(xpivot[channel], wMeter, r, WHITE);
        display.drawCircle(xpivot[channel], wMeter, r-2, BLACK);
    }
}

void drawTimeBlink(uint8_t cc) {
    int x = 2 + (2 * 25);
    if (32 == cc)
        display.fillRect(58, 0, 12, display.height() - 16, BLACK);
    else
        bigChar(cc, x, LCD25X44_LEN, 25, 44, lcd25x44);
}

void putVolume(bool v, char *buff) {
    putText(0, 0, buff);
    int w = 8;
    int start = v * w;
    uint8_t dest[w];
    memcpy(dest, volume8x8 + start, sizeof dest);
    display.drawBitmap(0, 0, dest, w, w, WHITE);
}

void putAudio(int a, char *buff) {
    char pad[32];
    sprintf(pad, "   %s  ", buff); // ensue we cleanup
    int x = maxXPixel()-(strlen(pad)*CHAR_WIDTH);
    putText(x, 0, pad);
    int w = 8;
    int start = a * w;
    uint8_t dest[w];
    memcpy(dest, volume8x8 + start, sizeof dest);
    display.drawBitmap(maxXPixel()-(w+2), 0, dest, w, w, WHITE);
}

void drawTimeText(char *buff) {
    display.fillRect(0, 0, display.width(), display.height() - 16, BLACK);
    // digit walk and "blit"
    int x = 2;
    for (size_t i = 0; i < strlen(buff); i++) {
        bigChar(buff[i], x, LCD25X44_LEN, 25, 44, lcd25x44);
        x += 25;
    }
}

void scrollerFinalize(void) {
    for (int line = 0; line < MAX_LINES; line++) {
        scroll[line].active = false;
        pthread_cancel(scroll[line].scrollThread);
        pthread_join(scroll[line].scrollThread, NULL);
        free(scroll[line].text);
    }
}

void baselineScroller(Scroller *s) {
    s->active = false;
    s->nystagma = true;
    s->lolimit = 1000;
    s->hilimit = -1000;
    strcpy(s->text, "");
    s->xpos = maxXPixel();
    s->forward = false;
    s->pause = false;
    s->textPix = 0;
}

void putScrollable(int line, char *buff) {
    scroll[line].active = false;
    int tlen = strlen(buff);
    bool goscroll = (maxCharacter() < tlen);
    if (!goscroll) {
        putTextToCenter(scroll[line].ypos, buff);
        scroll[line].textPix = -1;
    }
    else
    {
        baselineScroller(&scroll[line]);
        sprintf(scroll[line].text, " %s ", buff); // pad ends
        scroll[line].textPix = tlen * CHAR_WIDTH;
        scroll[line].active = true;
    }
    
}

#define SCAN_TIME 100
#define PAUSE_TIME 5000

void* scrollLine(void *input)
{
    sme *s;
    s = ((struct Scroller*)input);
    int timer = SCAN_TIME;
    //int testmin = 10000;
    //int testmax = -10000;
    while(true) {
        timer = SCAN_TIME;
        if (s->active) {
            // cylon sweep
            if (s->forward)
                s->xpos++;
            else
                s->xpos--;
            display.setTextWrap(false);
            clearLine(s->ypos);
            display.setCursor(s->xpos, s->ypos);
            display.print(s->text);

            if (-(CHAR_WIDTH/2) == s->xpos) {
            //if (0 == s->xpos) {
                if (!s->forward)
                    timer = PAUSE_TIME;
                s->forward = false;
            }

            if ((maxXPixel()-(int)(1.5*CHAR_WIDTH)) == ((s->textPix)+s->xpos))
                s->forward = true;

            // address annoying pixels
            display.fillRect(0, s->ypos, 2, CHAR_HEIGHT+2, BLACK);
            display.fillRect(maxXPixel()-2, s->ypos, 2, CHAR_HEIGHT+2, BLACK);

            // need to test for "nystagma" - where text is just shy
            // of being with static limits and gets to a point
            // where it rapidly bounces left to right and back again
            // more than annoying and needs to pin to xpos=0 and 
            // deactivate - implement test - check length limits
            // and travel test 
        }
        delay(timer);
    }   
}

void scrollerPause(void) {
    for (int line = 0; line < MAX_LINES; line++) {
        scroll[line].active = false;
    }
}

void scrollerInit(void) {
    for (int line = 0; line < MAX_LINES; line++) {
        if ((scroll[line].text =
                 (char *)malloc(MAXSCROLL_DATA * sizeof(char))) == NULL) {
            closeDisplay();
            return;
        } else {
            baselineScroller(&scroll[line]);
            scroll[line].ypos = line * (2+CHAR_HEIGHT);
            scroll[line].scrollMe = scrollLine;
            pthread_create(&scroll[line].scrollThread, NULL, scroll[line].scrollMe, (void *)&scroll[line]);
        }
    }
}

void drawTimeText2(char *buff, char *last) {
    // digit walk and "blit"
    int x = 2;
    for (size_t i = 0; i < strlen(buff); i++) {
        // selective updates, less "blink"
        if (buff[i] != last[i]) {
            display.fillRect(x, 1, 25, display.height() - 15, BLACK);
            bigChar(buff[i], x, LCD25X44_LEN, 25, 44, lcd25x44);
        }
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

void refreshDisplayScroller(void) {
    for (int line = 0; line < MAX_LINES; line++) {
        if (scroll[line].active) {
            display.display(); 
            break;
        }
    }
}

void putText(int x, int y, char *buff) {
    display.setTextSize(1);
    display.fillRect(x, y, (int16_t)strlen(buff) * CHAR_WIDTH, CHAR_HEIGHT, 0);
    display.setCursor(x, y);
    display.print(buff);
}

void clearLine(int y) { display.fillRect(0, y, maxXPixel(), CHAR_HEIGHT, 0); }

void putTextToCenter(int y, char *buff) {
    int tlen = strlen(buff);
    int px =
        maxCharacter() < tlen ? 0 : (maxXPixel() - (tlen * CHAR_WIDTH)) / 2;
    clearLine(y);
    putText(px, y, buff);
}

void testFont(int x, int y, char *buff) {
    //display.setTextFont(&LiberationMono_Regular6pt7bBitmaps);
    display.setTextSize(1);
    display.fillRect(x, y, (int16_t)strlen(buff) * CHAR_WIDTH, CHAR_HEIGHT, 0);
    display.setCursor(x, y);
    display.print(buff);
}

void drawRectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    display.drawRect(x, y, w, h, color);
}

// parked
void* scrollLineUgh(void *input)
{
    sme *s;
    s = ((struct Scroller*)input);
    int timer = SCAN_TIME;

    while(true) {
        timer = SCAN_TIME;

        if (s->active) {
            s->xpos--;
            if (s->xpos + s->textPix <= maxXPixel())
                s->xpos += s->textPix;
            display.setTextWrap(false);
            clearLine(s->ypos);
            // should appear to marquee - but sadly it does not???
            if (s->xpos > 0)
                display.setCursor(s->xpos - s->textPix, s->ypos);
            else
                display.setCursor(s->xpos + s->textPix, s->ypos);

            display.print(s->text);

            // address annoying pixels
            display.fillRect(0, s->ypos, 2, CHAR_HEIGHT+2, BLACK);
            display.fillRect(maxXPixel()-2, s->ypos, 2, CHAR_HEIGHT+2, BLACK);

            if (-3 == s->xpos) {
                s->forward = false; // what resets?
                timer = 5000;
            }
        }
        usleep(timer);
    }   
}

#endif
