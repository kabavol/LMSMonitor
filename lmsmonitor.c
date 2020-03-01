/*
 *	lmsmonitor.c
 *
 *	(c) 2015 László TÓTH
 *	(c) 2020 Stuart Hunter
 *
 *	Todo:	Done - Automatic server discovery
 *			Done - Get playerID automatically
 *			Reconnect to server
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
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <fcntl.h>
#include <net/if.h>
#include <sys/ioctl.h>

#include "common.h"
#include "display.h"
#include "sliminfo.h"
#include "tagUtils.h"
#include <sys/time.h>
#include <time.h>

#ifdef __arm__

// clang-format off
// retain this include order
#include "ArduiPi_OLED_lib.h"
#include "Adafruit_GFX.h"
#include "ArduiPi_OLED.h"
#include "display.h"
// clang-format on

#endif

#define VERSION "0.4.3"
#define SLEEP_TIME 100
#define CHRPIXEL 8

char stbl[BSIZE];
tag *tags;

static char *get_mac_address() {
    struct ifconf ifc;
    struct ifreq *ifr, *ifend;
    struct ifreq ifreq;
    struct ifreq ifs[3];

    uint8_t mac[6] = {0, 0, 0, 0, 0, 0};

    int sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd < 0)
        return NULL;

    ifc.ifc_len = sizeof(ifs);
    ifc.ifc_req = ifs;

    if (ioctl(sd, SIOCGIFCONF, &ifc) == 0) {
        // Get last interface.
        ifend = ifs + (ifc.ifc_len / sizeof(struct ifreq));

        // Loop through interfaces.
        for (ifr = ifc.ifc_req; ifr < ifend; ifr++) {
            if (ifr->ifr_addr.sa_family == AF_INET) {
                strncpy(ifreq.ifr_name, ifr->ifr_name, sizeof(ifreq.ifr_name));
                if (ioctl(sd, SIOCGIFHWADDR, &ifreq) == 0) {
                    memcpy(mac, ifreq.ifr_hwaddr.sa_data, 6);
                    // Leave on first valid address.
                    if (mac[0] + mac[1] + mac[2] != 0)
                        ifr = ifend;
                }
            }
        }
    }

    close(sd);

    char *macaddr = (char *)malloc(18);

    snprintf(macaddr, 18, "%02x:%02x:%02x:%02x:%02x:%02x", mac[0], mac[1],
             mac[2], mac[3], mac[4], mac[5]);

    return macaddr;
}

void signature(char *executable) {
    printf("\nThis is %s, compiled %s %s.\n\n", executable, __DATE__, __TIME__);
}

void print_help(char *executable) {
    printf("LMSMonitor Ver. %s\n"
           "Usage [options] -n Player name\n"
           "options:\n"
           " -r show remain time rather than track time\n"
           " -c display clock when not playing (Pi only)\n"
           " -v display visualization sequence when playing (Pi only)\n"
           " -t enable print info to stdout\n"
           " -i increment verbose level\n\n",
           VERSION);
    signature(executable);
}

int main(int argc, char *argv[]) {

    bool visualize = false;
    bool clock = false;
    bool extended = false;
    bool remaining = false;
    long lastVolume = -100;
    long actVolume = 0;
    long pTime, dTime, rTime;
    char buff[255];
    char *playerName = NULL;
    char lastBits[16] = "LMS";
    char lastTime[6] =
        "XX:XX"; // non-matching time digits (we compare one by one)!
    int aName;
    char *thatMAC = NULL;
    char *thisMAC = get_mac_address();

    bool refreshLMS = false;
    bool refreshClock = false;

    // clang-format off
	#define LINE_NUM 5
	tagtypes_t layout[LINE_NUM][3] = {
		{MAXTAG_TYPES, MAXTAG_TYPES, MAXTAG_TYPES},
		{COMPOSER,     ARTIST,       MAXTAG_TYPES},
		{ALBUM,        MAXTAG_TYPES, MAXTAG_TYPES},
		{TITLE,        MAXTAG_TYPES, MAXTAG_TYPES},
		{ALBUMARTIST,  CONDUCTOR,    MAXTAG_TYPES},
	};
    // clang-format on

    opterr = 0;
    while ((aName = getopt(argc, argv, "o:n:tvhric")) != -1) {
        switch (aName) {
            case 't': enableTOut(); break;

            case 'i': incVerbose(); break;

            case 'v': visualize = true; break;

            case 'c': clock = true; break;

            case 'n': playerName = optarg; break;

            case 'r': remaining = true; break;

            case 'h':
                print_help(argv[0]);
                exit(1);
                break;
        }
    }

    signature(argv[0]);
    thatMAC = player_mac();

    if (thatMAC != thisMAC) {
        extended = true;
        printf("Extended attrs from LMS enabled, no mimo!\n");
    }

#ifdef __arm__

    // init OLED display
    if (initDisplay() == EXIT_FAILURE) {
        exit(EXIT_FAILURE);
    }

    splashScreen();

#endif

    // init here - splash delay mucks the refresh flagging
    if ((tags = initSliminfo(playerName)) == NULL) {
        exit(1);
    }

    while (true) {

        if (isRefreshed()) {

            bool playing = (strcmp("play", tags[MODE].tagData) == 0);

            if (playing) {

                if (refreshLMS) {
#ifdef __arm__
                    resetDisplay(1);
#endif
                    refreshLMS = false;
                }
                refreshClock = true;

                tOut("_____________________\n");

                actVolume = tags[VOLUME].valid
                                ? strtol(tags[VOLUME].tagData, NULL, 10)
                                : 0;
                if (actVolume != lastVolume) {
                    if (0 == actVolume)
                        strncpy(buff, "  mute", 32);
                    else
                        sprintf(buff, "  %ld%% ", actVolume);
#ifdef __arm__
                    volume((0 != actVolume), buff);
#endif
                    lastVolume = actVolume;
                }

                // output sample rate and bit depth too, supports DSD nomenclature
                double samplerate =
                    tags[SAMPLERATE].valid
                        ? strtof(tags[SAMPLERATE].tagData, NULL) / 1000
                        : 44.1;
                int samplesize =
                    tags[SAMPLESIZE].valid
                        ? strtol(tags[SAMPLESIZE].tagData, NULL, 10)
                        : 16;
                if (1 == samplesize)
                    sprintf(buff, "DSD%.0f", (samplerate / 44.1));
                else
                    sprintf(buff, "%db/%.1fkHz", samplesize, samplerate);

                if (strstr(buff, ".0") != NULL) {
                    char *foo = NULL;
                    foo = replaceStr(buff, ".0", ""); // cleanup a little

                    sprintf(buff, "%s", foo);
                }

#ifdef __arm__
                if (strcmp(lastBits, buff) != 0) {
                    int bdlen = strlen(buff);
                    putText(60, 0, "       "); // prep
                    putText(maxXPixel() - (bdlen * CHAR_WIDTH) - 1, 0, buff);
                    strncpy(lastBits, buff, 32);
                }
#endif

                for (int line = 1; line < LINE_NUM; line++) {
#ifdef __arm__
                    int filled = false;
#endif

                    for (tagtypes_t *t = layout[line]; *t != MAXTAG_TYPES;
                         t++) {

                        if (tags[*t].valid) {
#ifdef __arm__
                            filled = true;
#endif
                            if (tags[*t].changed) {
                                strncpy(buff, tags[*t].tagData, 255); //maxCharacter());
#ifdef __arm__
                                putScrollable(line, buff);
#endif
                            }
                            sprintf(stbl, "%s\n", tags[*t].tagData);
                            tOut(stbl);
                            break;
                        }
                    }
#ifdef __arm__
                    if (!filled) {
                        clearLine(line * 10);
                    }
#endif
                }

                pTime = (tags[TIME].valid)
                            ? strtol(tags[TIME].tagData, NULL, 10)
                            : 0;
                dTime = (tags[DURATION].valid)
                            ? strtol(tags[DURATION].tagData, NULL, 10)
                            : 0;
                rTime = (tags[REMAINING].valid)
                            ? strtol(tags[REMAINING].tagData, NULL, 10)
                            : 0;

#ifdef __arm__
                sprintf(buff, "%02ld:%02ld", pTime / 60, pTime % 60);
                int tlen = strlen(buff);
                clearLine(56);
                putText(1, 56, buff);

                // this is limited to sub hour programing - a stream may be 1+ hours
                if (remaining)
                    sprintf(buff, "-%02ld:%02ld", rTime / 60, rTime % 60);
                else
                    sprintf(buff, "%02ld:%02ld", dTime / 60, dTime % 60);

                int dlen = strlen(buff);
                putText(maxXPixel() - (dlen * CHAR_WIDTH) - 1, 56, buff);

                sprintf(buff, "%s",
                        (tags[MODE].valid) ? tags[MODE].tagData : "");
                int mlen = strlen(buff);
                putText(
                    ((maxXPixel() - ((tlen + mlen + dlen) * CHAR_WIDTH)) / 2) +
                        (tlen * CHAR_WIDTH),
                    56, buff);

                drawHorizontalBargraph(
                    -1, 51, 0, 4, (pTime * 100) / (dTime == 0 ? 1 : dTime));
#else
                sprintf(buff, "%3ld:%02ld  %5s  %3ld:%02ld", pTime / 60,
                        pTime % 60,
                        (tags[MODE].valid) ? tags[MODE].tagData : "",
                        dTime / 60, dTime % 60);
                sprintf(stbl, "%s\n\n", buff);
                tOut(stbl);
#endif

                for (int i = 0; i < MAXTAG_TYPES; i++) {
                    tags[i].changed = false;
                }

#ifdef __arm__
                refreshDisplay();
#endif
            } else {

                scrollerPause(); // we need to re-activate too - save state!!!

                if (clock) {

                    if (refreshClock) {
#ifdef __arm__
                        resetDisplay(1);
                        strncpy(lastTime, "XX:XX", 6); // force, just in case
#endif
                        refreshClock = false;
                    }
                    refreshLMS = true;

                    struct timeval tv;
                    gettimeofday(&tv, NULL);
                    time_t now = tv.tv_sec;
                    struct tm loctm = *localtime(&now);

#ifdef __arm__

                    // time
                    sprintf(buff, "%02d:%02d", loctm.tm_hour, loctm.tm_min);
                    if (strcmp(lastTime, buff) != 0) {
                        // drawRectangle(0, 0, 128, 64, WHITE);
                        drawTimeText2(buff, lastTime);
                        strncpy(lastTime, buff, 32);
                    }

                    // colon (blink)
                    if (loctm.tm_sec % 2)
                        drawTimeBlink(32);
                    else
                        drawTimeBlink(58);

                    // seconds
                    drawHorizontalBargraph(
                        -1, 48, 0, 4,
                        (int)((100 *
                               (loctm.tm_sec + ((double)tv.tv_usec / 1000000)) /
                               60)));
                    // date
                    sprintf(buff, "  %d-%02d-%02d  ", loctm.tm_year + 1900,
                            loctm.tm_mon + 1, loctm.tm_mday);
                    putTextToCenter(56, buff);

                    // set changed so we'll repaint on play
                    for (int line = 1; line < LINE_NUM; line++) {
                        for (tagtypes_t *t = layout[line]; *t != MAXTAG_TYPES;
                             t++) {
                            if (tags[*t].valid) {
                                tags[*t].changed = true;
                            }
                        }
                    }
                    lastVolume = -1;
                    lastBits[0] = 0;
                    refreshDisplay();
#endif
                }
            }

            askRefresh();

        } // isRefreshed

        usleep(SLEEP_TIME);
        refreshDisplayScroller();

    } // main loop

#ifdef __arm__
    closeDisplay();
#endif
    closeSliminfo();
    return 0;
}
