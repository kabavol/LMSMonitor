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

#include "display.h"
#include "oledimg.h"
#include "libm6.h"

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
    display.clearDisplay();
    display.close();
    return;
}

void vumeter2upl(void) {
    //display.clearDisplay();
    display.drawBitmap(0, 0, vu2up128x64, 128, 64, WHITE);
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
    int x = 2 + (2 * 25);
    if (32 == cc)
        display.fillRect(58, 0, 12, display.height() - 16, BLACK);
    else
        bigChar(cc, x, LCD25X44_LEN, 25, 44, lcd25x44);
}

void volume(bool v, char *buff) {
    //display.fillRect(0, 0, (tlen+2) * CHAR_WIDTH, CHAR_HEIGHT, BLACK);
    putText(0, 0, buff);
    int w = 8;
    int start = v * w;
    uint8_t dest[w];
    memcpy(dest, volume8x8 + start, sizeof dest);
    display.drawBitmap(0, 0, dest, w, w, WHITE);
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

void putScrollable(int line, char *buff) {
    //scroll[line].scrollThread.active = false;
    scroll[line].active = false;
    int tlen = strlen(buff);
    bool goscroll = (maxCharacter() < tlen);
    if (!goscroll) {
        putTextToCenter(scroll[line].ypos, buff);
        scroll[line].textPix = -1;
    }
    else
    {
        sprintf(scroll[line].text, " %s ", buff); 
        scroll[line].xpos = maxXPixel();
        scroll[line].pause = false;
        scroll[line].forward = false;
        scroll[line].textPix = tlen * CHAR_WIDTH;
        scroll[line].active = true;
        // activate the scroller thread
        //scroll[line].scrollThread.active = true;
    }
    
}


void* scrollLineUgh(void *input)
{
    sme *s;
    s = ((struct Scroller*)input);
    int timer = 100;
    while(true) {
        timer = 100;
        if (s->active) {
            s->xpos--;
            if (s->xpos + s->textPix <= maxXPixel())
                s->xpos += s->textPix;
            display.setTextWrap(false);
            clearLine(s->ypos);
            // should appear to marquee - sadly it does not???
            if (s->xpos > 0)
                display.setCursor(s->xpos - s->textPix, s->ypos);
            else
                display.setCursor(s->xpos + s->textPix, s->ypos);

            display.print(s->text);

            if (0 == s->xpos) {
                s->forward = false;
                timer = 5000;
            }
        }
        usleep(timer);
    }   
}

void* scrollLine(void *input)
{
    sme *s;
    s = ((struct Scroller*)input);
    int timer = 100;
    //int testmin = 10000;
    //int testmax = -10000;
    while(true) {
        timer = 100;
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

            if (-3 == s->xpos) {
                if (!s->forward)
                    timer = 5000;
                s->forward = false;
            }

            if (maxXPixel() == ((s->textPix)+s->xpos))
                s->forward = true;

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
            strcpy(scroll[line].text, "");
            scroll[line].active = false;
            scroll[line].ypos = line * (2+CHAR_HEIGHT);
            scroll[line].xpos = maxXPixel();
            scroll[line].forward = false;
            scroll[line].pause = false;
            scroll[line].scrollMe = scrollLine;
            scroll[line].textPix = 0;
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

#endif
