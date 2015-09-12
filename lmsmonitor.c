/*
 *	lmsmonitor.c
 *
 *	(c) 2015 László TÓTH
 *
 *  V 0.3
 *
 *	Todo:	Auto reconect to server
			Compute "virtual" volume from LMS and soundcard volumes.
			Get LMS server IP from -s option
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#include "tagUtils.h"
#include "mixermon.h"
#include "sliminfo.h"
#include "display.h"
#include "common.h"

#ifdef __arm__

#include "ArduiPi_OLED_lib.h"
#include "Adafruit_GFX.h"
#include "ArduiPi_OLED.h"
#include "display.h"

#endif

#define SLEEP_TIME	(25000/25)
#define CHRPIXEL 8

char stbl[BSIZE];
tag *tags;

int main(int argc, char *argv[]) {
	long lastALSAVolume = 0;
	long  actALSAVolume = 0;
	long lastLMSVolume  = 0;
	long  actLMSVolume  = 0;
	long pTime, dTime;
	char buff[255];
	char *sndCard = NULL;
	char *playerName = NULL;
	int  aName;

	#define LINE_NUM 4
	tagtypes_t layout[LINE_NUM][3] = {
		{COMPOSER,    ARTIST,       MAXTAG_TYPES},
		{ALBUM,       MAXTAG_TYPES, MAXTAG_TYPES},
		{TITLE,       MAXTAG_TYPES, MAXTAG_TYPES},
		{ALBUMARTIST, CONDUCTOR,    MAXTAG_TYPES},
	};

	opterr = 0;
	while ((aName = getopt (argc, argv, "o:n:tvh")) != -1) {
		switch (aName) {
			case 't':
				enableTOut();
				break;

			case 'v':
				incVerbose();
				break;

			case 'o':
				sndCard = optarg;
				break;

			case 'n':
				playerName = optarg;
				break;

			case 'h':
				printf("LMSMonitor Ver. 0.3\nUsage [options] -n Player name\noptions:\n -o Soundcard (eg. hw:CARD=IQaudIODAC)\n -t enable print info to stdout\n -v increment verbose level\n\n");
				exit(1);
				break;
		}
	}

	if((tags = initSliminfo(playerName)) == NULL)	{ exit(1); }

	// init ALSA mixer monitor
	startMimo(sndCard, NULL);

#ifdef __arm__
	// init OLED display
	if (initDisplay() == EXIT_FAILURE) {
		exit(EXIT_FAILURE);
	}
#endif

	while (true) {

		if (tags[LMSVOLUME].valid) {
			actLMSVolume = strtol(tags[LMSVOLUME].tagData,(char **)NULL, 10);
		}
		actALSAVolume = getALSAVolume();

		if ((actALSAVolume != lastALSAVolume) || (actLMSVolume != lastLMSVolume)) {
			long virtVolume = (actALSAVolume * actLMSVolume) / 100;
			sprintf(buff, "Vol:             %3ld%%", virtVolume);
#ifdef __arm__
			putText(0, 0, buff);
			drawHorizontalBargraph(24, 2, 75, 2, actALSAVolume);
			drawHorizontalBargraph(24, 4, 75, 2, actLMSVolume);
			refreshDisplay();
#endif
			tOut(buff);
			lastALSAVolume = actALSAVolume;
			lastLMSVolume  = actLMSVolume;
		}

		if (isRefreshed()) {
			tOut("_____________________\n");

printf ("LMS mixer volume: %s\n", tags[LMSVOLUME].valid ? tags[LMSVOLUME].tagData : "");

			for (int line = 0; line < LINE_NUM; line++) {
#ifdef __arm__
				int filled = false;
#endif
				for (tagtypes_t *t = layout[line]; *t != MAXTAG_TYPES; t++) {
					if (tags[*t].valid) {
#ifdef __arm__
						filled = true;
#endif
						if (tags[*t].changed) {
								strncpy(buff, tags[*t].tagData, maxCharacter());
#ifdef __arm__
								putTextToCenter((line + 1) * 10, buff);
#endif
						}
						sprintf(stbl, "%s\n", tags[*t].tagData);
						tOut(stbl);
						break;
					}
				}
#ifdef __arm__
				if(!filled) {
					clearLine((line + 1) * 10);
				}
#endif
			}

			pTime = tags[TIME].valid     ? strtol(tags[TIME].tagData,     NULL, 10) : 0;
			dTime = tags[DURATION].valid ? strtol(tags[DURATION].tagData, NULL, 10) : 0;

#ifdef __arm__
			sprintf(buff, "%ld:%02ld", pTime/60, pTime%60);
			int tlen = strlen(buff);
			clearLine(56);
			putText(0, 56, buff);

			sprintf(buff, "%ld:%02ld", dTime/60, dTime%60);
			int dlen = strlen(buff);
			putText(maxXPixel() - (dlen * CHAR_WIDTH), 56, buff);

			sprintf(buff, "%s", tags[MODE].valid ? tags[MODE].tagData : "");
			int mlen = strlen(buff);
			putText(((maxXPixel() - ((tlen + mlen + dlen) * CHAR_WIDTH)) / 2) + (tlen * CHAR_WIDTH), 56, buff);

			drawHorizontalBargraph(-1, 51, 0, 4, (pTime*100) / (dTime == 0 ? 1 : dTime));
#else
			sprintf(buff, "%3ld:%02ld  %5s  %3ld:%02ld", pTime/60, pTime%60, tags[MODE].valid ? tags[MODE].tagData : "",  dTime/60, dTime%60);
			sprintf(stbl, "%s\n\n", buff);
			tOut(stbl);
#endif
			for(int i = 0; i < MAXTAG_TYPES; i++) {
				tags[i].changed = false;
			}

#ifdef __arm__
			refreshDisplay();
#endif
			askRefresh();
		}

		usleep(SLEEP_TIME);
    }

#ifdef __arm__
	closeDisplay();
#endif
	closeSliminfo();
	return 0;
}
