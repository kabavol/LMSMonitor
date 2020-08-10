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

// cache clock font
uint8_t *clockF = NULL;

void freeClockFont(void) {
    if (clockF)
        free(clockF);
}

void downsampleFont(const uint8_t *font, size_t s, int16_t w, int16_t h) {

    int16_t byteWidth = (w + 7) / 8; // Bitmap scanline pad whole bytes per line
    int byte = 0;

    int picture[w][h];

    // expand font to "bitmap" pixels (on/off)
    for (int16_t j = 0; j < h; j++) {
        for (int16_t i = 0; i < w; i++) {
            if (i & 7)
                byte <<= 1;
            else
                byte = font[j * byteWidth + i / 8];
            picture[i][j] = (byte & 0x80) ? 1 : 0;
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

    byteWidth = (dw + 7) / 8;
    double xscale = (dw + 0.0) / w;
    double yscale = (dh + 0.0) / h;
    double threshold = 0.5 / (xscale * yscale);
    double yend = 0.0;

    clockF = (uint8_t *)malloc(byteWidth * dh * sizeof(uint8_t));
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
                    sum += picture[x][y] * yportion *
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

}

const uint8_t *getOledFont(int font, bool h12) {
    if (h12) {
        size_t n = 0;
        uint16_t l = 13 * 44;
        uint16_t lp = 15 * 44;
        if(NULL==clockF) {
        switch (font) {
            case MON_FONT_CLASSIC:
            case MON_FONT_LCD2544:
                n = sizeof(lcd25x44) / sizeof(lcd25x44[0]);
                downsampleFont(lcd25x44, n, 25, l);
                break;
            case MON_FONT_DECOSOL:
                n = sizeof(soldeco25x44) / sizeof(soldeco25x44[0]);
                downsampleFont(soldeco25x44, n, 25, l);
                break;
            case MON_FONT_DECOHOL:
                n = sizeof(holdeco25x44) / sizeof(holdeco25x44[0]);
                downsampleFont(holdeco25x44, n, 25, l);
                break;
            case MON_FONT_FESTUS:
                n = sizeof(festus25x44) / sizeof(festus25x44[0]);
                downsampleFont(festus25x44, n, 25, lp);
                break;
            case MON_FONT_HOLFESTUS:
                n = sizeof(holfestus25x44) / sizeof(holfestus25x44[0]);
                downsampleFont(holfestus25x44, n, 25, lp);
                break;
            case MON_FONT_NB1999:
                n = sizeof(nb1999s25x44) / sizeof(nb1999s25x44[0]);
                downsampleFont(nb1999s25x44, n, 25, lp); // inc AM/PM
                break;
            case MON_FONT_ROBOTO:
                n = sizeof(roboto25x44) / sizeof(roboto25x44[0]);
                downsampleFont(roboto25x44, n, 25, lp); // inc AM/PM
                break;
            case MON_FONT_NOTO2544:
                n = sizeof(noto25x44) / sizeof(noto25x44[0]);
                downsampleFont(noto25x44, n, 25, l);
                break;
            case MON_FONT_NOTF2544:
                n = sizeof(notof25x44) / sizeof(notof25x44[0]);
                downsampleFont(notof25x44, n, 25, l);
                break;
            case MON_FONT_COLT2544:
                n = sizeof(colby25x44) / sizeof(colby25x44[0]);
                downsampleFont(colby25x44, n, 25, lp); // inc AM/PM
                break;
            case MON_FONT_TTYP2544:
                n = sizeof(ttypongo25x44) / sizeof(ttypongo25x44[0]);
                downsampleFont(ttypongo25x44, n, 25, lp);
                break;
            case MON_FONT_SHLP2544:
                n = sizeof(shleep25x44) / sizeof(shleep25x44[0]);
                downsampleFont(shleep25x44, n, 25, l);
                break;
                // these will never be exercised
            case MON_FONT_LCD1521: return lcd15x21; break;
            case MON_FONT_LCD1217: return lcd12x17; break;
            case MON_FONT_LCD2348: return lcd23x48; break;
            default:
                n = sizeof(lcd25x44) / sizeof(lcd25x44[0]);
                downsampleFont(lcd25x44, n, 25, l);
        }
        }
        return clockF;
    } else {
        switch (font) {
            case MON_FONT_CLASSIC:
            case MON_FONT_LCD2544: return lcd25x44; break;
            case MON_FONT_DECOSOL: return soldeco25x44; break;
            case MON_FONT_DECOHOL: return holdeco25x44; break;
            case MON_FONT_FESTUS: return festus25x44; break;
            case MON_FONT_HOLFESTUS: return holfestus25x44; break;
            case MON_FONT_NB1999: return nb1999s25x44; break;
            case MON_FONT_ROBOTO: return roboto25x44; break;
            case MON_FONT_NOTO2544: return noto25x44; break;
            case MON_FONT_NOTF2544: return notof25x44; break;
            case MON_FONT_COLT2544: return colby25x44; break;
            case MON_FONT_TTYP2544: return ttypongo25x44; break;
            case MON_FONT_SHLP2544: return shleep25x44; break;
            case MON_FONT_LCD1521: return lcd15x21; break;
            case MON_FONT_LCD1217: return lcd12x17; break;
            case MON_FONT_LCD2348: return lcd23x48; break;
            default: return lcd25x44;
        }
    }
    return lcd25x44;
}

void printOledFontTypes(void) {
    // display clock fonts only
    fprintf(stdout, "\nOLED Clock Fonts:\n");
    for (int font = 0; font < MON_FONT_LCD1521; font++)
        fprintf(stdout, "   %2d ...: %s\n", font, oled_font_str[font]);
}