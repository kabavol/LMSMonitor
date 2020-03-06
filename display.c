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
    // VU Mode

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
        display.fillCircle(xpivot[channel], wMeter, r-4, BLACK);
    }
}

// caps and previous state
bin_chan_t caps;
bin_chan_t last_bin;
bin_chan_t last_caps;

void stereoSpectrum(struct vissy_meter_t *vissy_meter) 
{
    // SA mode

    int bins = 12;
	int wsa = maxXPixel()-2;
	int hsa = maxYPixel()-2;

	//int wbin = wsa / (2 + (bins * 2)); // 12 bar display
	int wbin = wsa / ((bins+1) * 2); // 12 bar display

	// SA scaling
	double multiSA = (double)hsa / 31.00; // max input is 31 -2 to leaves head-room

    for (int channel = 0; channel < 2; channel++) {
		//int ofs = int(wbin/2) + 2 + (channel * ((wsa+2) / 2));
		int ofs = int(wbin/2) + (channel * ((wsa+2) / 2));
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

			display.fillRect(ofs+(bin*wbin), hsa-lob, wbin-1, lob, BLACK);
			display.fillRect(ofs+(bin*wbin), hsa-oob, wbin-1, oob, WHITE);
            last_bin.bin[channel][bin] = vissy_meter->sample_bin_chan[channel][bin];

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
				coot = int(multiSA * last_caps.bin[channel][bin]);
    			display.fillRect(ofs+(bin*wbin), hsa-coot, wbin-1, 1, BLACK);
			}
			if (caps.bin[channel][bin] > 0) {
				coot = int(multiSA * caps.bin[channel][bin]);
    			display.fillRect(ofs+(bin*wbin), hsa-coot, wbin-1, 1, WHITE);
			}

            last_caps.bin[channel][bin] = caps.bin[channel][bin];

        }
    }
    // finesse
    display.fillRect(0, maxYPixel()-2, maxXPixel(), 2, BLACK);

}

void stereoPeak(struct vissy_meter_t *vissy_meter) {
    /*
    	w, h := 162, 40 // ls.vulayout.w2m, ls.vulayout.h2m
	var iconMem = new(bytes.Buffer)

	level := [12]int32{-50, -40, -30, -24, -18, -12, -9, -6, -3, 0, 2, 4}
	ypos := [2]float64{9.28, 25.21}

	bs := `<svg width="162" height="40" xmlns="http://www.w3.org/2000/svg">

	     <defs>
	       <g id="led">
		   <rect x="0" y="0" width="8" height="5.5" />
		   <rect x=".5" y=".5" width="1.25" height="4.5" fill="black" fill-opacity=".15" stroke-width=".2" stroke-opacity=".5" stroke-position="inside" stroke="darkgray"/>
		   <rect x="2.33" y=".5" width="1.25" height="4.5" fill="black" fill-opacity=".15" stroke-width=".2" stroke-opacity=".5" stroke-position="inside" stroke="darkgray"/>
		   <rect x="4.13" y=".5" width="1.25" height="4.5" fill="black" fill-opacity=".15" stroke-width=".2" stroke-opacity=".5" stroke-position="inside" stroke="darkgray"/>
		   <rect x="6.0" y=".5" width="1.25" height="4.5" fill="black" fill-opacity=".15" stroke-width=".2" stroke-opacity=".5" stroke-position="inside" stroke="darkgray"/>
			  </g>
	       <path id="over" fill-opacity="0.4" d="m136.51396,17.79571c0.720987,0 1.231205,0.184043 1.530655,0.552737c0.29945,0.368694 0.362619,0.90503 0.188295,1.609617c-0.177969,0.704587 -0.508396,1.240924 -0.990067,1.609617c-0.485922,0.368694 -1.089073,0.552737 -1.81006,0.552737c-0.720987,0 -1.231205,-0.184043 -1.530655,-0.552737c-0.29945,-0.364442 -0.36019,-0.900778 -0.182221,-1.609617c0.174325,-0.708839 0.502322,-1.245176 0.983993,-1.609617c0.485922,-0.368694 1.089073,-0.552737 1.81006,-0.552737zm-0.255109,1.002215c-0.287302,0 -0.526618,0.086859 -0.716735,0.261183c-0.194369,0.174325 -0.327998,0.408782 -0.400886,0.704587l-0.097184,0.388738c-0.072888,0.295805 -0.056488,0.530263 0.048592,0.704587c0.105081,0.174325 0.301879,0.261183 0.589181,0.261183c0.287302,0 0.52844,-0.086859 0.722809,-0.261183c0.198621,-0.174325 0.334072,-0.408782 0.40696,-0.704587l0.097184,-0.388738c0.072888,-0.295805 0.054666,-0.530263 -0.054666,-0.704587c-0.109333,-0.174325 -0.307953,-0.261183 -0.595255,-0.261183zm4.604113,3.249605l-1.554951,0l-0.43733,-4.178931l1.433471,0l0.139703,2.794053l0.024296,0l1.542803,-2.794053l1.37273,0l-2.520722,4.178931zm2.034192,0l1.044733,-4.178931l3.614047,0l-0.255109,1.002215l-2.271686,0l-0.139703,0.577033l1.943689,0l-0.242961,0.959696l-1.943689,0l-0.157925,0.637773l2.314205,0l-0.249035,1.002215l-3.656565,0l-0.000001,-0.000001zm9.074598,-2.897311c-0.064992,0.255109 -0.190117,0.485922 -0.37659,0.692439c-0.190117,0.206517 -0.429434,0.358368 -0.716735,0.455552l0.491996,1.74932l-1.506359,0l-0.358368,-1.524581l-0.49807,0l-0.382664,1.524581l-1.34236,0l1.044733,-4.178931l2.557166,0c0.29945,0 0.540588,0.058918 0.722809,0.176147c0.182221,0.113584 0.303701,0.267257 0.364442,0.461626c0.056488,0.198621 0.056488,0.413034 0,0.643847zm-1.378804,0.054666c0.024296,-0.109333 0.010326,-0.200443 -0.042518,-0.273331c-0.056488,-0.072888 -0.13788,-0.109333 -0.242961,-0.109333l-0.880734,0l-0.188295,0.771402l0.880734,0c0.105081,0 0.202265,-0.036444 0.291553,-0.109333c0.092933,-0.07714 0.153673,-0.170073 0.182221,-0.279405z"/>
	   </defs>

	`
	tick := `<use xlink:href="#led" x="%f" y="%f" fill="%s" fill-opacity="%f" stroke-width="0.1" stroke="darkslategrey"/>`
	for p, l := range level {
		xpos := float64(24.00 + ((float64(p) + 1.00) * 9.791538))
		for channel := 0; channel < 2; channel++ {
			color := `green`
			opacity := .9
			testd := (20.00 + dBfs[channel])
			if l >= testd {
				color = `lightgrey`
				opacity = .4
			} else {
				if l >= 0 && testd > 0 {
					if l >= 4 {
						color = `red`
					} else {
						color = `yellow`
					}
				}
			}
			bs += fmt.Sprintf("\n"+tick, xpos, ypos[channel], color, opacity)
		}
	}

	// Overload
	style := `style="fill:lightgrey;fill-opacity:0.5"`
	if dBfs[0] > 0 || dBfs[1] > 0 {
		style = `style="fill:red;fill-opacity:1"`
	}
	bs += fmt.Sprintf("\n<use xlink:href=\"#over\" %s/>", style)

	bs += " </svg>"

	iconMem.WriteString(bs)

	iconI, err := oksvg.ReadIconStream(iconMem)
	if err != nil {
		fmt.Println(iconMem.String())
		panic(err)
	}

	img := image.NewRGBA(image.Rect(0, 0, ls.vulayout.w2m, ls.vulayout.h2m))
	gv := rasterx.NewScannerGV(w, h, img, img.Bounds())
	r := rasterx.NewDasher(w, h, gv)
	iconI.SetTarget(0, 0, float64(ls.vulayout.w2m), float64(ls.vulayout.h2m))
	iconI.Draw(r, 1.0)

	dc := gg.NewContext(ls.vulayout.w2m, ls.vulayout.h2m)
	dc.DrawImageAnchored(ls.vulayout.baseImage, 0, 0, 0, 0)
	dc.DrawImageAnchored(img, 0, 0, 0, 0)

	ls.mux.Lock()
	draw.Draw(ls.vulayout.vu, ls.vulayout.vu.Bounds(), dc.Image(), image.ZP, draw.Over)
	ls.mux.Unlock()
*/

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
        if (scroll[line].text)
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
