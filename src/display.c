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

#include <math.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "display.h"
#include "oledimg.h"
#include "visualize.h"

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#define PI acos(-1.0000)
#define DOWNMIX 0

#ifdef __arm__

// clang-format off
// retain this include order
#include "ArduiPi_OLED_lib.h"
#include "Adafruit_GFX.h"
#include "ArduiPi_OLED.h"
#include "lmsmonitor.h"
#ifdef CAPTURE_BMP
#include "bmpfile.h"
#endif
// clang-format on

ArduiPi_OLED display;
// default oled type
int oledType = OLED_SH1106_I2C_128x64;
int8_t oledAddress = 0x00;
int8_t icons[NUMNOTES][4];
uint16_t _char_width = 6;
uint16_t _char_height = 8;
short scrollMode = SCROLL_MODE_CYLON;
sme scroll[MAX_LINES];

int maxCharacter(void) { return (int)(maxXPixel()/_char_width); }
int maxLine(void) { return (int)(maxYPixel()/_char_height); }
uint16_t charWidth(void) { return _char_width; }
uint16_t charHeight(void){ return _char_height; }

#else
int maxCharacter(void) { return 21; }
int maxLine(void) { return MAX_LINES; }
uint16_t charWidth(void) { return 6; }
uint16_t charHeight(void)  { return 8; }

#endif

int maxXPixel(void) { return 128; }
int maxYPixel(void) { return 64; }
double deg2Rad(double angDeg) { return (PI * angDeg / 180.0); }
double rad2Deg(double angRad) { return (180.0 * angRad / PI); }

#ifdef __arm__

void fontMetrics(void)
{
    int16_t  x, y, x1, y1;
    display.getTextBounds("W", 0, 0, &x1, &y1, &_char_width, &_char_height);
}

void printFontMetrics(void)
{
    fontMetrics();
    char stb[BSIZE] = {0};
    sprintf(stb, "%s %d (px)\n%s %d (px)\n", 
        labelIt("GFX Font Width", LABEL_WIDTH, "."), 
        _char_width,
        labelIt("GFX Font Height", LABEL_WIDTH, "."), 
        _char_height);
    putMSG(stb, LL_INFO);

}

void printOledSetup(void)
{
    char stb[BSIZE] = {0};
    if (0 == oledAddress)
        oledAddress = display.getOledAddress();
    
    sprintf(stb, "%s (%d) %s\n%s 0x%x\n", 
        labelIt("OLED Driver", LABEL_WIDTH, "."), 
        oledType,
        oled_type_str[oledType],
        labelIt("OLED Address", LABEL_WIDTH, "."), 
        oledAddress);
    putMSG(stb, LL_INFO);
}

void printScrollerMode(void)
{
    char stb[50] = {0};
    sprintf(stb, "%s (%d) %s\n", 
        labelIt("Scrolling Mode", LABEL_WIDTH, "."), 
        scrollMode,
        scrollerMode[scrollMode]);
    putMSG(stb, LL_INFO);
}

void printOledTypes(void) {
    printf("Supported OLED types:\n");
    for (int i = 0; i < OLED_LAST_OLED; i++)
        if (strstr(oled_type_str[i], "128x64"))
            if (oledType == i)
                fprintf(stdout, "    %1d* ..: %s\n", i, oled_type_str[i]);
            else
                fprintf(stdout, "    %1d ...: %s\n", i, oled_type_str[i]);
    printf("\n* is default\n");
}

bool setOledType(int ot) {
    if (ot < 0 || ot >= OLED_LAST_OLED || !strstr(oled_type_str[ot], "128x64"))
        return false;
    oledType = ot;
    return true;
}

bool setOledAddress(int8_t oa) {
    if (oa <= 0 )
        return false;
    oledAddress = oa;
    return true;
}

void setScrollMode(short sm)
{
    if ((sm >= SCROLL_MODE_CYLON)&&(sm < SCROLL_MODE_MAX))
        if (sm != scrollMode)
            scrollMode = sm;
}

void bigChar(uint8_t cc, int x, int y, int len, int w, int h, const uint8_t font[],
             uint16_t color) {
    // need fix for space, and minus sign
    int start = (cc - 48) * len;
    uint8_t dest[len];
    memcpy(dest, font + start, sizeof dest);
    display.drawBitmap(x, y, dest, w, h, color); // -1 - breathing room
}

void resetDisplay(int fontSize) {
    display.fillScreen(BLACK);
    display.setTextSize(fontSize);
    display.setTextColor(WHITE);
    display.setTextWrap(false);
}

void softClear(void) {
    display.fillScreen(BLACK);
}

void displayBrightness(int bright) { display.setBrightness(bright); }

int initDisplay(void) {

    // IIC specific - what about SPI ??

    // if strstr(SPI) else
    if (0 != oledAddress)
    {
        if (!display.init(OLED_I2C_RESET, oledType, oledAddress)) {
            return EXIT_FAILURE;
        }
    }
    else
    {
        if (!display.init(OLED_I2C_RESET, oledType)) {
            return EXIT_FAILURE;
        }
    }

    display.begin();
    display.setFont(&NotoSans_Regular5pt7b);
    display.setFont();
    fontMetrics();
    display.setBrightness(0);
    resetDisplay(1);
    scrollerInit();
    return 0;
}

void closeDisplay(void) {
    scrollerFinalize();
    clearDisplay();
    display.close();
}

void clearDisplay() {
    display.clearDisplay();
    display.display();
}

void vumeter2upl(void) {
    softClear();
    display.drawBitmap(0, 0, vu2up128x64, 128, 64, WHITE);
}

void vumeterDownmix(bool inv) {
    //if (inv)
    //    softClear();  // display.drawBitmap(0, 0, vudm128x64, 128, 64, BLACK);
    if (inv)
        display.drawBitmap(0, 0, vudm128x64, 128, 64, BLACK);
    else
        display.drawBitmap(0, 0, vudm128x64, 128, 64, WHITE);
}

void peakMeterH(bool inv) {
    //if (inv)
    //    softClear();  // display.drawBitmap(0, 0, peak_rms, 128, 64, BLACK);
    if (inv)
        display.drawBitmap(0, 0, peak_rms, 128, 64, BLACK);
    else
        display.drawBitmap(0, 0, peak_rms, 128, 64, WHITE);
}

void splashScreen(void) {
    display.clearDisplay();
    display.drawBitmap(0, 0, splash, 128, 64, WHITE);
    display.display();
    for (int i = 0; i < MAX_BRIGHTNESS; i++) {
        display.setBrightness(i);
        dodelay(10);
    }
    dodelay(180);
}

meter_chan_t lastVU = {-1000, -1000};
meter_chan_t overVU = {0, 0};
meter_chan_t dampVU = {-1000, -1000};

void downmixVU(struct vissy_meter_t *vissy_meter) {

    vumeterDownmix(true);

    double hMeter = (double)maxYPixel() + 2.00;
    double rMeter = (double)hMeter - 5.00;
    double wMeter = (double)maxXPixel();
    double xpivot = wMeter / 2.00;
    double rad = (180.00 / PI);  // 180/pi
    double divisor = 0.00;
    double mv_downmix = 0.000;   // downmix meter position

    int16_t ax;
    int16_t ay;

    if (dampVU.metric[DOWNMIX] > -1000)
    {
        ax = (int16_t)(xpivot + (sin(dampVU.metric[DOWNMIX] / rad) * rMeter));
        ay = (int16_t)(hMeter - (cos(dampVU.metric[DOWNMIX] / rad) * rMeter));

        display.drawLine((int16_t)xpivot - 1, (int16_t)hMeter, ax, ay, BLACK);
        display.drawLine((int16_t)xpivot,     (int16_t)hMeter, ax, ay, BLACK);
        display.drawLine((int16_t)xpivot + 1, (int16_t)hMeter, ax, ay, BLACK);
    }

    vumeterDownmix(false);

    meter_chan_t thisVU = {
        vissy_meter->sample_accum[0],
        vissy_meter->sample_accum[1]};

    for (int channel = 0; channel < 2; channel++) {
        // meter value
        mv_downmix += (double)thisVU.metric[channel];
        divisor++;
    }

    mv_downmix /= divisor;
    mv_downmix /= 58.000;
    mv_downmix -= 48.000;

    // damping
    if (dampVU.metric[DOWNMIX] <= mv_downmix)
        dampVU.metric[DOWNMIX] = mv_downmix;
    else
        dampVU.metric[DOWNMIX] -= 4;
    if (dampVU.metric[DOWNMIX] < mv_downmix)
        dampVU.metric[DOWNMIX] = mv_downmix;

    ax = (int16_t)(xpivot + (sin(dampVU.metric[DOWNMIX] / rad) * rMeter));
    ay = (int16_t)(hMeter - (cos(dampVU.metric[DOWNMIX] / rad) * rMeter));

    // thick needle with definition
    display.drawLine((int16_t)xpivot - 2, (int16_t)hMeter, ax, ay, BLACK);
    display.drawLine((int16_t)xpivot - 1, (int16_t)hMeter, ax, ay, WHITE);
    display.drawLine((int16_t)xpivot,     (int16_t)hMeter, ax, ay, WHITE);
    display.drawLine((int16_t)xpivot + 1, (int16_t)hMeter, ax, ay, WHITE);
    display.drawLine((int16_t)xpivot + 2, (int16_t)hMeter, ax, ay, BLACK);

    // finesse
    display.fillRect((int16_t)xpivot - 3, maxYPixel() - 6, maxXPixel() / 2, 6,
                     BLACK);

    uint16_t r = 7;
    display.fillCircle((int16_t)xpivot, (int16_t)hMeter, r, WHITE);
    display.drawCircle((int16_t)xpivot, (int16_t)hMeter, r - 2, BLACK);
    display.fillCircle((int16_t)xpivot, (int16_t)hMeter, r - 4, BLACK);

}

void stereoVU(struct vissy_meter_t *vissy_meter, char *downmix) {

    meter_chan_t thisVU = {
        vissy_meter->sample_accum[0],
        vissy_meter->sample_accum[1]};

    // overload : if n # samples > 0dB light overload

    // do no work if we don't need to
    if ((lastVU.metric[0] == thisVU.metric[0]) &&
        (lastVU.metric[1] == thisVU.metric[1]))
        return;

    // VU Mode
    if (strncmp(downmix, "N", 1) != 0) {
        downmixVU(vissy_meter);
        return;
    }

    lastVU.metric[0] = thisVU.metric[0];
    lastVU.metric[1] = thisVU.metric[1];

    vumeter2upl();

    double hMeter = (double)maxYPixel() + 2;
    double rMeter = (double)hMeter - 8.00;
    double wMeter = (double)maxXPixel() / 2.00;
    int16_t xpivot[2];
    xpivot[0] = maxXPixel() / 4;
    xpivot[1] = xpivot[0] * 3;
    double rad = (180.00 / PI); // 180/pi

    // meter positions

    for (int channel = 0; channel < 2; channel++) {

        // meter value
        double mv =
            (double)lastVU.metric[channel] / 58.00;
        mv -= 36.000; // zero adjust [-3dB->-20dB]

        // damping
        if (dampVU.metric[channel] <= mv)
            dampVU.metric[channel] = mv;
        else
            dampVU.metric[channel] -= 2;
        if (dampVU.metric[channel] < mv)
            dampVU.metric[channel] = mv;

        int16_t ax = (int16_t)(xpivot[channel] + (sin(dampVU.metric[channel] / rad) * rMeter));
        int16_t ay = (int16_t)(hMeter          - (cos(dampVU.metric[channel] / rad) * rMeter));

        // thick needle with definition
        display.drawLine(xpivot[channel] - 2, (int16_t)hMeter, ax, ay, BLACK);
        display.drawLine(xpivot[channel] - 1, (int16_t)hMeter, ax, ay, WHITE);
        display.drawLine(xpivot[channel],     (int16_t)hMeter, ax, ay, WHITE);
        display.drawLine(xpivot[channel] + 1, (int16_t)hMeter, ax, ay, WHITE);
        display.drawLine(xpivot[channel] + 2, (int16_t)hMeter, ax, ay, BLACK);
    }

    // finesse
    display.fillRect(xpivot[0] - 3, maxYPixel() - 6, maxXPixel() / 2, 6, BLACK);

    uint16_t r = 7;
    for (int channel = 0; channel < 2; channel++) {
        display.fillCircle(xpivot[channel], (int16_t)hMeter, r, WHITE);
        display.drawCircle(xpivot[channel], (int16_t)hMeter, r - 2, BLACK);
        display.fillCircle(xpivot[channel], (int16_t)hMeter, r - 4, BLACK);
    }

}

// caps and previous state
bin_chan_t caps;
bin_chan_t last_bin;
bin_chan_t last_caps;

void downmixSpectrum(struct vissy_meter_t *vissy_meter) {

    int bins = 12;
    int wsa = maxXPixel() - 2;
    int hsa = maxYPixel() - 4;

    double wbin = (double)wsa / (double)(bins + 1); // 12 bar display, downmixed
    //wbin = 9.00;                                    // fix

    display.fillRect(1, 1, (int)wbin, hsa, BLACK);
    display.fillRect(wsa-6, 1, (int)wbin, hsa, BLACK);

    // SA scaling
    double multiSA = (double)hsa / 31.00;

    int ofs = int(wbin * 0.75);
    //ofs = 6; // fix

    for (int bin = 0; bin < bins; bin++) {

        int divisor = 0;
        double test = 0.00;
        double lob = (multiSA / 2.00);
        double oob = (multiSA / 2.00);

        // erase - last and its rounding errors leaving phantoms
        display.fillRect(ofs -1 + (int)(bin * wbin), 0, (int)wbin + 2, hsa, BLACK);

        if (vissy_meter->is_mono) {
            test += (double)vissy_meter->sample_bin_chan[0][bin];
            divisor++;
        } else {
            for (int channel = 0; channel < 2; channel++) {
                if (bin < vissy_meter->numFFT[channel]) {
                    test += (double)vissy_meter->sample_bin_chan[channel][bin];
                    divisor++;
                }
            }
        }
        test /= (double)divisor;
        oob = (multiSA * test);

        if (last_bin.bin[DOWNMIX][bin] > 0)
            lob = (int)(multiSA * last_bin.bin[DOWNMIX][bin]);

        display.fillRect(ofs + (int)(bin * wbin), hsa - (int)lob, (int)wbin - 1, (int)lob,
                         BLACK);
        display.fillRect(ofs + (int)(bin * wbin), hsa - (int)oob, (int)wbin - 1, (int)oob,
                         WHITE);
        last_bin.bin[DOWNMIX][bin] = (int)test;

        if (test >= caps.bin[DOWNMIX][bin]) {
            caps.bin[DOWNMIX][bin] = (int)test;
        } else if (caps.bin[DOWNMIX][bin] > 0) {
            caps.bin[DOWNMIX][bin]--;
            if (caps.bin[DOWNMIX][bin] < 0) {
                caps.bin[DOWNMIX][bin] = 0;
            }
        }

        int coot = 0;
        if (last_caps.bin[DOWNMIX][bin] > 0) {
            if (last_bin.bin[DOWNMIX][bin] < last_caps.bin[DOWNMIX][bin]) {
                coot = (int)(multiSA * last_caps.bin[DOWNMIX][bin]);
                display.fillRect(ofs + (int)(bin * wbin), hsa - coot, (int)wbin - 1,
                                 1, BLACK);
            }
        }
        if (caps.bin[DOWNMIX][bin] > 0) {
            coot = (int)(multiSA * caps.bin[DOWNMIX][bin]);
            display.fillRect(ofs + (int)(bin * wbin), hsa - coot, (int)wbin - 1, 1,
                             WHITE);
        }

        last_caps.bin[DOWNMIX][bin] = caps.bin[DOWNMIX][bin];
    }

    // finesse
    display.fillRect(0, maxYPixel() - 4, maxXPixel(), 4, BLACK);
    // track detail scroller...
}

void stereoSpectrum(struct vissy_meter_t *vissy_meter, char *downmix) {

    // SA mode
    if (strncmp(downmix, "N", 1) != 0) {
        downmixSpectrum(vissy_meter);
        return;
    }

    int bins = 12;
    int wsa = maxXPixel() - 2;
    int hsa = maxYPixel() - 4;

    // 2.5.5.5 ...
    //int wbin = wsa / (2 + (bins * 2)); // 12 bar display
    double wbin = (double)wsa / (double)((bins + 1) * 2); // 12 bar display
    wbin = 5.00;                                          // fix

    // SA scaling
    double multiSA =
        (double)hsa / 31.00; // max input is 31 -2 to leaves head-room

    for (int channel = 0; channel < 2; channel++) {

        //int ofs = int(wbin/2) + 2 + (channel * ((wsa+2) / 2));
        int ofs = int(wbin * 0.75) + (channel * ((wsa + 2) / 2));
        ofs = 2 + (channel * ((wsa + 2) / 2)); // fix
        for (int bin = 0; bin < bins; bin++) {
            double test = 0.00;
            int lob = (int)(multiSA / 2.00);
            int oob = (int)(multiSA / 2.00);
            if (bin < vissy_meter->numFFT[channel]) {
                test = (double)vissy_meter->sample_bin_chan[channel][bin];
                oob = (int)(multiSA * test);
            }

            if (last_bin.bin[channel][bin])
                lob = int(multiSA * last_bin.bin[channel][bin]);

            display.fillRect(ofs + (int)(bin * wbin), hsa - lob, wbin - 1, lob,
                             BLACK);
            display.fillRect(ofs + (int)(bin * wbin), hsa - oob, wbin - 1, oob,
                             WHITE);
            last_bin.bin[channel][bin] =
                vissy_meter->sample_bin_chan[channel][bin];

            if (test >= caps.bin[channel][bin]) {
                caps.bin[channel][bin] = test;
            } else if (caps.bin[channel][bin] > 0) {
                caps.bin[channel][bin]--;
                if (caps.bin[channel][bin] < 0) {
                    caps.bin[channel][bin] = 0;
                }
            }

            int coot = 0;
            if (last_caps.bin[channel][bin] > 0) {
                if (last_bin.bin[channel][bin] < last_caps.bin[channel][bin]) {
                    coot = int(multiSA * last_caps.bin[channel][bin]);
                    display.fillRect(ofs + (int)(bin * wbin), hsa - coot,
                                     wbin - 1, 1, BLACK);
                }
            }
            if (caps.bin[channel][bin] > 0) {
                coot = int(multiSA * caps.bin[channel][bin]);
                display.fillRect(ofs + (int)(bin * wbin), hsa - coot, wbin - 1,
                                 1, WHITE);
            }

            last_caps.bin[channel][bin] = caps.bin[channel][bin];
        }
    }

    // finesse
    display.fillRect(0, maxYPixel() - 4, maxXPixel(), 4, BLACK);

    // track detail scroller...
}

void ovoidSpectrum(struct vissy_meter_t *vissy_meter, char *downmix) {

    // a spectrum that is centered and horizontal

    /*
   left hi-lo right lo-hi
        +++++ +++++
      +++++++ ++++++
    +++++++++ ++++++++
      +++++++ ++++++
       ++++++ ++++++
         ++++ +++++++
    */

    int bins = 12;

    int wsa = (maxXPixel() - 6) / 2;
    int hsa = maxYPixel() - 2;

    double hbin = (double)hsa / (double)(bins + 1); // 12 bar display

    // SA scaling
    double multiSA =
        (double)wsa / 31.00; // max input is 31

    for (int channel = 0; channel < 2; channel++) {

        int ofs = 2;
        int mod = (0 == channel)?-1:1;
        for (int bin = 0; bin < bins; bin++) {
            int lob = (int)(multiSA / 2.00);
            int oob = (int)(multiSA / 2.00);
            if (bin < vissy_meter->numFFT[channel])
                oob = (int)(multiSA * (double)vissy_meter->sample_bin_chan[channel][bin]);

            if (last_bin.bin[channel][bin])
                lob = int(multiSA * last_bin.bin[channel][bin]);
            else
                lob = oob;
            display.fillRect((wsa + mod) + ((channel-1) * lob), ofs, lob, hbin - 1,
                            BLACK);
            display.fillRect((wsa + mod) + ((channel-1) * oob), ofs, oob, hbin - 1,
                            WHITE);

            last_bin.bin[channel][bin] =
                vissy_meter->sample_bin_chan[channel][bin];

            ofs += hbin;

        }
    }

}

meter_chan_t lastPK = {-1000, -1000};
void stereoPeakH(struct vissy_meter_t *vissy_meter, char *downmix) {

    // intermediate variable so we can easily switch metrics
    meter_chan_t meter = {
        vissy_meter->sample_accum[0],
        vissy_meter->sample_accum[1]};

    // do no work if we don't need to
    if ((lastPK.metric[0] == meter.metric[0]) && (lastPK.metric[1] == meter.metric[1]))
        return;

    peakMeterH(true);

    int level[19] = {-36, -30, -20, -17, -13, -10, -8, -7, -6, -5,
                     -4,  -3,  -2,  -1,  0,   2,   3,  5,  8};

    uint8_t zpos = 16;
    uint8_t xpos = zpos;
    uint8_t ypos[2] = {7, 40};
    size_t ll = sizeof(level) / sizeof(level[0]);
    size_t p = 0;

    peakMeterH(false); // 🎵

    for (int *l = level; (p < ll); l++) {
        uint8_t nodeo = (*l < 0) ? 5 : 7;
        uint8_t nodew = (*l < 0) ? 2 : 4;
        for (int channel = 0; channel < 2; channel++) {
            // meter value
            double mv = -48.00 + ((double)lastPK.metric[channel] / 48.00);
            if ((mv >= (double)*l) && (lastPK.metric[channel] > meter.metric[channel])){
                display.fillRect(xpos, ypos[channel], nodew, 18, BLACK);
            }
        }
        xpos += nodeo;
        p++;
    }

    xpos = zpos;
    p = 0;

    for (int *l = level; (p < ll); l++) {
        uint8_t nodeo = (*l < 0) ? 5 : 7;
        uint8_t nodew = (*l < 0) ? 2 : 4;
        for (int channel = 0; channel < 2; channel++) {
            // meter value
            double mv = -48.00 + ((double)meter.metric[channel] / 48.00);
            if (mv >= (double)*l) {
                display.fillRect(xpos, ypos[channel], nodew, 18, WHITE);
            }
        }
        xpos += nodeo;
        p++;
    }

    lastPK.metric[0] = meter.metric[0];
    lastPK.metric[1] = meter.metric[1];

}

void drawTimeBlinkS(uint8_t cc, int y)
{
    int w = 15;
    int h = 21;
    int x = 49 + (2 * w);    
    if (32 == cc) // a space - colon off
        bigChar(':', x, y, LCD15X21_LEN, w, h, microlcd, BLACK);
    else
        bigChar(cc, x, y, LCD15X21_LEN, w, h, microlcd, WHITE);
}

void drawTimeBlinkL(uint8_t cc, int y) {
    int w = 25;
    int h = 44;
    int x = 2 + (2 * w);    
    if (32 == cc) // a space - colon off
        bigChar(':', x, y, LCD25X44_LEN, w, h, lcd25x44, BLACK);
    else
        bigChar(cc, x, y, LCD25X44_LEN, w, h, lcd25x44, WHITE);
}

void drawTimeTextS(char *buff, char *last, int y) 
{
    // digit walk and "blit"
    int x = 49;
    int w = 15;
    int h = 21;
    for (size_t i = 0; i < strlen(buff); i++) {
        // selective updates, less "blink"
        if (buff[i] != last[i]) {
            if ('X' == last[i])
                display.fillRect(x, y-1, w, h+2, BLACK);
            else
                bigChar(last[i], x, y, LCD15X21_LEN, w, h, microlcd, BLACK); // soft erase
            bigChar(buff[i], x, y, LCD15X21_LEN, w, h, microlcd, WHITE);
        }
        x += w;
    }
}

void drawTimeTextL(char *buff, char *last, int y) {
    // digit walk and "blit"
    int x = 2;
    int w = 25;
    int h = 44;
    for (size_t i = 0; i < strlen(buff); i++) {
        // selective updates, less "blink"
        if (buff[i] != last[i]) {
            if ('X' == last[i])
                display.fillRect(x, 0, w, display.height() - 14, BLACK);
            else
                bigChar(last[i], x, y, LCD25X44_LEN, w, h, lcd25x44, BLACK); // soft erase
            bigChar(buff[i], x, y, LCD25X44_LEN, w, h, lcd25x44, WHITE);
        }
        x += w;
    }
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
    int x = maxXPixel() - (strlen(pad) * _char_width);
    putText(x, 0, pad);
    int w = 8;
    int start = a * w;
    uint8_t dest[w];
    memcpy(dest, volume8x8 + start, sizeof dest);
    display.drawBitmap(maxXPixel() - (w + 2), 0, dest, w, w, WHITE);
}

void scrollerFinalize(void) {
    for (int line = 0; line < maxLine(); line++) {
        pthread_cancel(scroll[line].scrollThread);
        pthread_join(scroll[line].scrollThread, NULL);
        if (scroll[line].text)
            free(scroll[line].text);
        pthread_mutex_destroy(&scroll[line].scrollox);
    }
}

void baselineScroller(Scroller *s) {
    short sm = scrollMode;
    if(SCROLL_MODE_RANDOM == scrollMode) {
        srandom(time(NULL));
        sm = random() % SCROLL_MODE_MAX;
    }
    s->active = false;
    s->initialized = true;
    s->nystagma = true;
    s->lolimit = 1000;
    s->hilimit = -1000;
    s->xpos = maxXPixel()+1;
    s->forward = false;
    s->scrollMode = sm;
    s->textPix = -1;
    if (NULL != s->text)
        strcpy(s->text, "");
}

void clearScrollable(int line) {
    char buff[2] = {0};
    putScrollable(line, buff);
}

bool acquireLock(const int line)
{
    char buff[128] = {0};
    bool ret = true;
    if (NULL != scroll[line].text) // looking for instantiation here
    {
        uint8_t test = 0;
        while (pthread_mutex_trylock(&scroll[line].scrollox) != 0)
        {
            if (test>30)
            {
                ret = false;
                sprintf(buff,"scroller[%d] mutex acquire failed\n", line);
                putMSG(buff, LL_DEBUG);
                break;
            }
            usleep(10);
            test++;
        }
    }
    return ret;
}

bool putScrollable(int line, char *buff) {

    setScrollActive(line, false);
    bool ret = true;
    if ((line > 0 )&&(line < maxLine()))
    {
        int tlen = strlen(buff);
        bool goscroll = (maxCharacter() < tlen);
        if (!goscroll) {
            putTextToCenter(line * (2 + _char_height), buff); // fast!
            if (scroll[line].text)
            {
                if (acquireLock(line))
                {
                    strcpy(scroll[line].text, "");
                    scroll[line].textPix = -1;
                    pthread_mutex_unlock(&scroll[line].scrollox);
                }
            }
            ret = false;
        } else {
            if (scroll[line].text)
            {
                if (acquireLock(line))
                {
                    baselineScroller(&scroll[line]);
                    sprintf(scroll[line].text, " %s ", buff); // pad ends
                    scroll[line].textPix = tlen * _char_width;
                    scroll[line].active = true;
                    short sm = scrollMode;
                    if(SCROLL_MODE_RANDOM == scrollMode) {
                        srandom(time(NULL));
                        sm = random() % SCROLL_MODE_MAX;
                    }
                    scroll[line].scrollMode = sm;
                    pthread_mutex_unlock(&scroll[line].scrollox);
                }
            }
        }
    }
    return ret;
}

#define SCAN_TIME 100
#define PAUSE_TIME 5000

// ?? thread safe ??
void *scrollLine(void *input) {

    sme *s;
    s = ((struct Scroller *)input);
    int timer = SCAN_TIME;
    display.setTextWrap(false);
    while (true) {
        timer = SCAN_TIME;
        if (s->active) {

            if (acquireLock(s->line))
            {
                if ((int)strlen(s->text) > maxCharacter()) {

                    switch (s->scrollMode) {
                    case SCROLL_MODE_INFSIN:
                        putTextToCenter(s->ypos, s->text);
                        sinisterRotate(s->text);
                        timer = 3 * SCAN_TIME;
                        break;

                    case SCROLL_MODE_INFDEX:
                        putTextToCenter(s->ypos, s->text);
                        dexterRotate(s->text);
                        timer = 3 * SCAN_TIME;
                        break;

                    default:
                        // cylon sweep
                        if (s->forward)
                            s->xpos++;
                        else
                            s->xpos--;
                        clearLine(s->ypos);
                        display.setCursor(s->xpos, s->ypos);
                        display.print(s->text);

                        if (-(_char_width / 2) == s->xpos) {
                            //if (0 == s->xpos) {
                            if (!s->forward)
                                timer = PAUSE_TIME;
                            s->forward = false;
                        }

                        if ((maxXPixel() - (int)(1.5 * _char_width)) ==
                            ((s->textPix) + s->xpos))
                            s->forward = true;

                        // need to test for "nystagma" - where text is just shy
                        // of being with static limits and gets to a point
                        // where it rapidly bounces left to right and back again
                        // more than annoying and needs to pin to xpos=0 and
                        // deactivate - implement test - check length limits
                        // and travel test

                    }

                    // address annoying pixels
                    display.fillRect(0, s->ypos, 2, _char_height + 2, BLACK);
                    display.fillRect(maxXPixel() - 2, s->ypos, 2, _char_height + 2,
                                    BLACK);
                }
                pthread_mutex_unlock(&s->scrollox);
            }else printf("cannot update %d\n",s->line);
        }
        dodelay(timer);
    }
}

void setScrollActive(int line, bool active) {
    if (acquireLock(line))
    {
        if (active != scroll[line].active) {
            scroll[line].active = active;
        }
        pthread_mutex_unlock(&scroll[line].scrollox);
    }
}

void scrollerPause(void) {
    if (activeScroller()) {
        for (int line = 0; line < maxLine(); line++) {
            setScrollActive(line, false);
        }
    }
}

void scrollerInit(void) {
    for (int line = 0; line < maxLine(); line++) {
        if (pthread_mutex_init(&scroll[line].scrollox, NULL) != 0) {
            closeDisplay();
            printf("\nscroller[%d] mutex init has failed\n", line);
            return;
        }
        if ((scroll[line].text =
                 (char *)malloc(MAXSCROLL_DATA * sizeof(char))) == NULL) {
            closeDisplay();
            printf("scroller malloc error\n");
            return;
        } else {
            baselineScroller(&scroll[line]);
            scroll[line].ypos = line * (2 + _char_height);
            scroll[line].line = line;   // dang, dumb to have missed this !?!
            scroll[line].scrollMe = scrollLine;
            scroll[line].scrollMode = scrollMode;
            scroll[line].active = false;
            pthread_create(&scroll[line].scrollThread, NULL,
                           scroll[line].scrollMe, (void *)&scroll[line]);
        }
    }
}

bool activeScroller(void) {
    bool ret = false;
    for (int line = 0; line < maxLine(); line++) {
        if (acquireLock(line))
        {
            if (scroll[line].active)
                ret = true;
            pthread_mutex_unlock(&scroll[line].scrollox);
        }
        if (ret)
            break;
    }
    return ret;
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

#ifdef CAPTURE_BMP

bool snapOn = false;
void setSnapOn(void) { snapOn = true; }
void setSnapOff(void) { snapOn = false; }

int soff = 0;
void shotAndDisplay(void) {
    int16_t h = maxYPixel();
    int16_t w = maxXPixel();

    // read internal buffer and dump to bitmap
    if (snapOn) {

        char snapfile[32];

        sprintf(snapfile, "snap%05d.bmp", soff);
        bmpfile_t *bmp;

        rgb_pixel_t pixelW = {255, 255, 255, 255};
        rgb_pixel_t pixelB = {0, 0, 0, 255};
        int depth = 8; // 1 bit iis just fine

        if ((bmp = bmp_create(w, h, depth)) != NULL) {
            for (int16_t y = 0; y < h; y++) {
                for (int16_t x = 0; x < w; x++) {
                    if (display.readPixel(x, y))
                        bmp_set_pixel(bmp, x, y, pixelW);
                    else
                        bmp_set_pixel(bmp, x, y, pixelB);
                }
            }
            bmp_save(bmp, snapfile);
            bmp_destroy(bmp);
        }
    }

    if (soff > 2000)
        setSnapOff();
    else
        soff++;

    // finally display
    display.display();
}

void refreshDisplay(void) { shotAndDisplay(); }
#else
void refreshDisplay(void) { display.display(); }
#endif

void refreshDisplayScroller(void) {
    if (activeScroller())
        display.display();
}

void putText(int x, int y, char *buff) {
    display.setTextSize(1);
    display.fillRect(x, y, (int16_t)strlen(buff) * _char_width, _char_height, 0);
    display.setCursor(x, y);
    display.print(buff);
}

void clearLine(int y) { display.fillRect(0, y, maxXPixel(), _char_height, 0); }

void putTextCenterColor(int y, char *buff, uint16_t color) {
    int tlen = strlen(buff);
    int px =
        maxCharacter() < tlen ? 0 : (maxXPixel() - (tlen * _char_width)) / 2;
    clearLine(y);
    display.setTextColor(color);
    putText(px, y, buff);
    if (color != WHITE)
        display.setTextColor(WHITE);
}

void putTextToCenter(int y, char *buff) {
    int tlen = strlen(buff);
    int px =
        maxCharacter() < tlen ? 0 : (maxXPixel() - (tlen * _char_width)) / 2;
    clearLine(y);
    putText(px, y, buff);
}

void drawRectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    display.drawRect(x, y, w, h, color);
}

// rudimentry screen saver - only kicks in
// if the user does not specify [c]lock

void nagSaverSetup(void) {
    srandom(time(NULL));

    // initialize
    for (uint8_t n = 0; n < NUMNOTES; n++) {
        icons[n][XPOS] = random() % display.width();
        icons[n][YPOS] = -1 * (random() % NAGNOTE_HEIGHT);
        icons[n][DELTAY] = random() % 5 + 1;
        icons[n][DELTAX] =
            ((random() % 3 + (1 * (random() % 2)) == 1) ? -1 : 1);
    }
}

void nagSaverNotes(void) {

    int w = NAGNOTE_WIDTH;
    int h = NAGNOTE_HEIGHT;

    for (uint8_t n = 0; n < NUMNOTES; n++) {
        display.drawBitmap(icons[n][XPOS], icons[n][YPOS], nag_notes, w, h,
                           WHITE);
    }

    display.display();
    usleep(200);

    for (uint8_t n = 0; n < NUMNOTES; n++) {
        display.drawBitmap(icons[n][XPOS], icons[n][YPOS], nag_notes, w, h,
                           BLACK);

        icons[n][XPOS] += icons[n][DELTAX];
        icons[n][YPOS] += icons[n][DELTAY];
        if (icons[n][YPOS] > display.height()) {
            icons[n][XPOS] = random() % display.width();
            icons[n][YPOS] = -1 * (random() % NAGNOTE_HEIGHT);
            icons[n][DELTAY] = random() % 5 + 1;
            icons[n][DELTAX] =
                ((random() % 3 + (1 * (random() % 2)) == 1) ? -1 : 1);
        }
    }
}

#endif