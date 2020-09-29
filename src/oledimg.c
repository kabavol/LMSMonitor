/*
 *	oledimg.c
 *
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

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

#include "oledimg.h"
#include "fauxfont.h"

// cache clock font
uint8_t *clockF = NULL;

void freeClockFont(void) {
    if (clockF)
        free(clockF);
}

// interpolated
void downsampleFontMurated1bit(const uint8_t *font, size_t s, int16_t w,
                               int16_t h, iscale_t scale) {

    int threshold = 128;
    int16_t bpl = (w + 7) / 8; // Bitmap scanline pad whole "bytes per line"
    int byte = 0;

    uint8_t *fbits;
    fbits = (uint8_t *)malloc(w * h * sizeof(uint8_t));
    uint8_t pW = 255;
    uint8_t pB = 0;

    int z = 0;
    // expand "font" to "bitmap" pixels (on/off)
    for (int16_t j = 0; j < h; j++) {
        for (int16_t i = 0; i < w; i++) {
            if (i & 7)
                byte <<= 1;
            else
                byte = font[j * bpl + i / 8];
            fbits[z] = (byte & 0x80) ? pW : pB; // on or off
            z++;
        }
    }

    int dw = (scale.xscale > 0) ? (int)(w * scale.xscale) : w;
    if ((0 != scale.xlimit) && (dw > scale.xlimit))
        dw = (int)scale.xlimit; // pin
    int dh = (scale.yscale > 0) ? (int)(h * scale.yscale) : h;
    if ((0 != scale.ylimit) && (dh > scale.ylimit))
        dh = (int)scale.ylimit; // pin

    bpl = (dw + 7) / 8; // Bitmap scanline pad whole "bytes per line"
    uint8_t agg = 0;
    bool ok = false;
    uint16_t retf1c = 0;
    uint16_t idx = 0;

    // allocate memory for "cached" font as bytes
    clockF = (uint8_t *)malloc(bpl * dh * sizeof(uint8_t));

    float d1, d2, d3, d4;
    uint8_t p1, p2, p3, p4; // nearby (murated) pixels

    for (int i = 0; i < dh; i++) {

        byte = 7;
        agg = 0;
        ok = false;

        for (int j = 0; j < dw; j++) {

            float tmp = (float)(i) / (float)(dh - 1) * (h - 1);
            int l = (int)floor(tmp);
            if (l < 0) {
                l = 0;
            } else {
                if (l >= h - 1) {
                    l = h - 2;
                }
            }

            float u = tmp - l;
            tmp = (float)(j) / (float)(dw - 1) * (w - 1);
            int c = (int)floor(tmp);
            if (c < 0) {
                c = 0;
            } else {
                if (c >= w - 1) {
                    c = w - 2;
                }
            }
            float t = tmp - c;

            // coefficients
            d1 = (1 - t) * (1 - u);
            d2 = t * (1 - u);
            d3 = t * u;
            d4 = (1 - t) * u;

            // walled pixels
            p1 = *((uint8_t *)fbits + (l * w) + c);
            p2 = *((uint8_t *)fbits + (l * w) + c + 1);
            p3 = *((uint8_t *)fbits + ((l + 1) * w) + c + 1);
            p4 = *((uint8_t *)fbits + ((l + 1) * w) + c);

            // "color" components
            uint8_t cc;
            cc = (uint8_t)(p1 * d1) + (uint8_t)(p2 * d2) + (uint8_t)(p3 * d3) +
                 (uint8_t)(p4 * d4);

            ok = true;
            if (cc > threshold) {
                agg += pow(2, byte);
            }

            byte--;

            if (idx != 0 && (0 == ((idx + 1) % (dw)))) {
                byte = -1;
            }

            // roll over
            if (byte < 0) {
                byte = 7;
                clockF[retf1c] = agg;
                retf1c++;
                agg = 0;
                ok = false;
            }

            idx++;
        }
        // in flight - snag 'em
        if (ok) {
            clockF[retf1c] = agg;
            retf1c++;
            ok = false; // safe
        }
    }

    free(fbits);
}

/*
org_img[20][30]; --monochrome values
sampled_img[10][15];

for(int i=0; i < 10; i++)
{
    for(int j=0; j < 15; j++)
    {
        int average = org_img[2*i][2*j] + org_img[2*i+1][2*j]+ org_img[2*i][2*j+1] + org_img[2*i+1][2*j+1];
        average = average>>2; --integer division by 4.
        sampled_img[i][j] = average;
    }
*/

// naive downsample
void downsampleFont(const uint8_t *font, size_t s, int16_t w, int16_t h,
                    iscale_t scale) {

    int16_t bpl = (w + 7) / 8; // Bitmap scanline pad whole "bytes per line"
    int byte = 0;

    int fbits[w][h];

    // expand font to "bitmap" pixels (on/off)
    for (int16_t j = 0; j < h; j++) {
        for (int16_t i = 0; i < w; i++) {
            if (i & 7)
                byte <<= 1;
            else
                byte = font[j * bpl + i / 8];
            fbits[i][j] = (byte & 0x80) ? 1 : 0;
        }
    }

    // downscale in x only - we'll want to fix this to <= 18px
    float fct = 0.80;

    int dw = (int)(w * fct);
    if (dw > 18)
        dw = 18; // pin
    int dh = h;  // pin - (int)(h * fct);

    bool ok = false;
    int idx = 0;
    int agg = 0;

    bpl = (dw + 7) / 8;
    double xscale = (dw + 0.0) / w;
    double yscale = (dh + 0.0) / h;
    double threshold = 0.5 / (xscale * yscale);
    double yend = 0.0;

    clockF = (uint8_t *)malloc(bpl * dh * sizeof(uint8_t));
    uint16_t retf1c = 0;

    for (int f = 0; f < dh; f++) { // y on output

        byte = 7;
        agg = 0;
        ok = false;

        double ystart = yend;
        yend = (f + 1) / yscale;
        if (yend >= h)
            yend = h - 0.000001;
        double xend = 0.0;
        for (int g = 0; g < dw; g++) { // x on output
            double xstart = xend;
            xend = (g + 1) / xscale;
            if (xend >= w)
                xend = w - 0.000001;
            double sum = 0.0;
            for (int y = (int)ystart; y <= (int)yend; ++y) {
                double yportion = 1.0;
                if (y == (int)ystart)
                    yportion -= ystart - y;
                if (y == (int)yend)
                    yportion -= y + 1 - yend;
                for (int x = (int)xstart; x <= (int)xend; ++x) {
                    double xportion = 1.0;
                    if (x == (int)xstart)
                        xportion -= xstart - x;
                    if (x == (int)xend)
                        xportion -= x + 1 - xend;
                    sum += fbits[x][y] * yportion *
                           xportion; // can use "raw" bit here ???
                }
            }

            ok = true;
            if (sum > threshold) {
                agg += pow(2, byte);
            }

            byte--;

            if (idx != 0 && (0 == ((idx + 1) % (dw)))) {
                byte = -1;
            }

            // roll over
            if (byte < 0) {
                byte = 7;
                clockF[retf1c] = agg;
                retf1c++;
                agg = 0;
                ok = false;
            }

            idx++;
        }

        // in flight - snag 'em
        if (ok) {
            clockF[retf1c] = agg;
            retf1c++;
            ok = false; // safe
        }
    }

    free(fbits);
}

#define USE_DOWNSCALE_ALGO downsampleFontMurated1bit

const uint8_t *fontPassthru(int font) {
    switch (font) {
        case MON_FONT_CLASSIC:
        case MON_FONT_LCD2544: return lcd25x44z(); break;
        case MON_FONT_DECOSOL: return soldeco25x44z(); break;
        case MON_FONT_DECOHOL: return holdeco25x44z(); break;
        case MON_FONT_FESTUS: return festus25x44z(); break;
        case MON_FONT_HOLFESTUS: return holfestus25x44z(); break;
        case MON_FONT_NB1999: return nb1999s25x44z(); break;
        case MON_FONT_ROBOTO: return roboto25x44z(); break;
        case MON_FONT_NOTO2544: return noto25x44z(); break;
        case MON_FONT_NOTF2544: return notof25x44z(); break;
        case MON_FONT_COLT2544: return colby25x44z(); break;
        case MON_FONT_TTYP2544: return ttypongo25x44z(); break;
        case MON_FONT_SHLP2544: return shleep25x44z(); break;
        case MON_FONT_LCD1521: return lcd15x21; break;
        case MON_FONT_LCD1217: return lcd12x17; break;
        //case MON_FONT_LCD2348: return lcd23x48; break;
        default: return lcd25x44z();
    }
    return lcd25x44z();
}
const uint8_t *getOledFont(int font, bool h12) {

    uint16_t w = 25;
    uint16_t h = 44;
    uint16_t l = 13 * h;
    uint16_t lp = 15 * h;
    iscale_t scale = {0.8, 1.0, 18, 0};
    uint8_t *scratch = (uint8_t *)fontPassthru(font);
    size_t n = sizeof(scratch) / sizeof(scratch[0]);

    if (NULL == clockF) {
        switch (font) {
            case MON_FONT_CLASSIC:
            case MON_FONT_LCD2544:
                if (h12) {
                    USE_DOWNSCALE_ALGO(scratch, n, w, l, scale);
                } else {
                    clockF = scratch;
                }
                break;
            case MON_FONT_DECOSOL:
            case MON_FONT_DECOHOL:
            case MON_FONT_FESTUS:
            case MON_FONT_HOLFESTUS:
            case MON_FONT_NB1999:
            case MON_FONT_ROBOTO:
            case MON_FONT_NOTO2544:
            case MON_FONT_NOTF2544:
            case MON_FONT_COLT2544:
            case MON_FONT_TTYP2544:
            case MON_FONT_SHLP2544:
                if (h12) {
                    USE_DOWNSCALE_ALGO(scratch, n, w, lp, scale);
                } else {
                    clockF = scratch;
                }
                break;
            case MON_FONT_LCD1521: return lcd15x21; break;
            case MON_FONT_LCD1217: return lcd12x17; break;
            //case MON_FONT_LCD2348: return lcd23x48; break;
            default: USE_DOWNSCALE_ALGO(scratch, n, w, l, scale);
        }
    }
    return clockF;
}

void printOledFontTypes(void) {
    // display clock fonts only
    fprintf(stdout, "\nOLED Clock Fonts:\n");
    for (int font = 0; font < MON_FONT_LCD1521; font++)
        fprintf(stdout, "   %2d ...: %s\n", font, oled_font_str[font]);
}