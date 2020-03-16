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

#include <signal.h>
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
#include "vizsse.h"
#include "vizshmem.h"
#include "visualize.h"
#include "timer.h"

// clang-format on

#include <ctype.h>
// missing under linux
void strupr(char *s)
{
    char *p = s;
    while (*p) {
        *p = toupper(*p);
        ++p;
    }
} 
#endif

#define SLEEP_TIME 200
#define CHRPIXEL 8
#define LINE_NUM 5

char stbl[BSIZE];
tag *tags;
bool playing = false;

// clang-format off
tagtypes_t layout[LINE_NUM][3] = {
	{MAXTAG_TYPES, MAXTAG_TYPES, MAXTAG_TYPES},
	{COMPOSER,     ARTIST,       MAXTAG_TYPES},
	{ALBUM,        MAXTAG_TYPES, MAXTAG_TYPES},
	{TITLE,        MAXTAG_TYPES, MAXTAG_TYPES},
	{ALBUMARTIST,  CONDUCTOR,    MAXTAG_TYPES},
};
// clang-format on


// free and cleanup here
void before_exit(void) {
    printf("\nCleanup and shutdown\n");
    closeSliminfo();
#ifdef __arm__
    clearDisplay();
    closeDisplay();
    vissySSEFinalize();
    vissySHMEMFinalize();
    timer_finalize(); // should be all set ???
#endif
    printf("All Done\nBye Bye.\n");
}

void sigint_handler(int sig) {
  before_exit();
  exit(0);
}

void attach_signal_handler(void) {

  struct sigaction new_action, old_action;

  new_action.sa_handler = sigint_handler;
  sigemptyset (&new_action.sa_mask);
  new_action.sa_flags = 0;

  sigaction (SIGINT, NULL, &old_action);
  if (old_action.sa_handler != SIG_IGN)
    sigaction (SIGINT, &new_action, NULL);
  sigaction (SIGHUP, NULL, &old_action);
  if (old_action.sa_handler != SIG_IGN)
    sigaction (SIGHUP, &new_action, NULL);
  sigaction (SIGTERM, NULL, &old_action);
  if (old_action.sa_handler != SIG_IGN)
    sigaction (SIGTERM, &new_action, NULL);
  sigaction (SIGQUIT, NULL, &old_action);
  if (old_action.sa_handler != SIG_IGN)
    sigaction (SIGQUIT, &new_action, NULL);
  sigaction (SIGSTOP, NULL, &old_action);
  if (old_action.sa_handler != SIG_IGN)
    sigaction (SIGSTOP, &new_action, NULL);

  new_action.sa_flags = SA_NODEFER;
  sigaction(SIGSEGV, &new_action, NULL); /* catch seg fault - and hopefully backtrace */

}

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
    printf("This is %s, compiled %s %s.\n", executable, __DATE__, __TIME__);
}

void print_help(char *executable) {
    printf("%s Ver. %s\n"
           "Usage [options] -n Player name\n"
           "options:\n"
           " -r show remain time rather than track time\n"
           " -c display clock when not playing (Pi only)\n"
           " -v display visualization sequence when playing (Pi only)\n"
           " -t enable print info to stdout\n"
           " -z no splash screen\n"
           " -i increment verbose level\n\n",
           APPNAME, VERSION);
    signature(executable);
}

#ifdef __arm__
void setupPlayMode(void)
{
    for (int line = 1; line < LINE_NUM; line++) {
        for (tagtypes_t *t = layout[line]; *t != MAXTAG_TYPES;
             t++) {
            if (tags[*t].valid) {
                tags[*t].changed = true;
            }
        }
    }
}

void toggleVisualize(size_t timer_id, void *user_data) {
    if ((bool)user_data) {
        if (playing) {
                if (isVisualizeActive()) {
                    deactivateVisualizer();
                    setupPlayMode();
                } else {
                    scrollerPause(); // we need to re-activate too - save state!!!
                    activateVisualizer();
                }
                clearDisplay();
            }
    }
}

#endif

int main(int argc, char *argv[]) {

    attach_signal_handler();

    bool visualize = false;
    bool clock = false;
    bool extended = false;
    bool remaining = false;
    bool splash = true;
    long actVolume = 0;
    int audio = 3; // 2 HD 3 SD 4 DSD
    long pTime, dTime, rTime;
    char buff[255];
    char *playerName = NULL;
    char meterMode[3];
    int aName;
    char *thatMAC = NULL;
    char *thisMAC = get_mac_address();

    bool refreshLMS = false;
    bool refreshClock = false;

    long lastVolume = -100;
    char lastBits[16] = "LMS";
    char lastTime[6] = "XX:XX"; // non-matching time digits (we compare one by one)!

#ifdef __arm__
    Options opt;
#endif

    opterr = 0;
    while ((aName = getopt(argc, argv, "n:m:tivcrzhp")) != -1) {
        switch (aName) {
            case 'n': playerName = optarg; break;

            case 'm': strncpy(meterMode, optarg, 2); break; // need to support a list

            case 't': enableTOut(); break;

            case 'i': incVerbose(); break;

#ifdef __arm__
            case 'p': sscanf(optarg, "%d", &opt.port); break;
#endif
            case 'v': visualize = true; break;

            case 'c': clock = true; break;

            case 'r': remaining = true; break;

            case 'z': splash = false; break;

            case 'h':
                print_help(argv[0]);
                exit(1);
                break;
        }
    }

#ifdef __arm__

    if (!(meterMode))
        strncpy(meterMode, MODE_VU, 2);
    else
    {
        strupr(meterMode);
        // validate and silently assign to default if "bogus"
        if ((strncmp(meterMode, MODE_VU, 2) != 0) 
        &&(strncmp(meterMode, MODE_SA, 2) != 0)
        &&(strncmp(meterMode, MODE_PK, 2) != 0)) {
            strncpy(meterMode, MODE_VU, 2);
        }
    }
    
    // init OLED display
    if (initDisplay() == EXIT_FAILURE) {
        exit(EXIT_FAILURE);
    }
    // throw up splash if not nixed by caller
    if (splash)
        splashScreen();

#endif

    signature(argv[0]);
    thatMAC = player_mac();

    // init here - splash delay mucks the refresh flagging
    if ((tags = initSliminfo(playerName)) == NULL) {
        exit(1);
    }

    // init for visualize mechanism here
    if (thatMAC != thisMAC) {
        extended = true;
        printf("Extended attrs from LMS enabled, no MIMO!\n");
#ifdef __arm__
        if (visualize) {
            // go SSE for visualizer
            opt.host = getPlayerIP();
            if(!opt.port)
                opt.port = 8022;
            opt.endpoint = "/visionon?subscribe=SA-VU";
            visualize = setupSSE(opt);
        }
#endif
    }
#ifdef __arm__
    else
    {
        if (visualize) {
            // go SHMEM for visualizer
            visualize = setupSHMEM(argc, argv);
        }
    }

    if(!visualize)
        putMSG("Visualization is not active", LL_INFO);
    // if we had a clean setup
    // init visualizer mode and
    // timer toggle
    if (visualize) {
        setVisMode(meterMode); // need a user specified cycle mode on this
        size_t viztimer;
        timer_initialize();
        viztimer = timer_start(60*1000, toggleVisualize, TIMER_PERIODIC, (void*)visualize);
    }

#endif


#ifdef __arm__
    clearDisplay(); // clear splash if shown
    // bitmap capture
    //setSnapOn();
    //setSnapOff();
#endif

    // re-work this so as to "pages" metaphor
    while (true) {

        if (isRefreshed()) {

            playing = (strcmp("play", tags[MODE].tagData) == 0);

            if (playing) {

                if (refreshLMS) {
#ifdef __arm__
                    resetDisplay(1);
#endif
                    refreshLMS = false;
                }
                refreshClock = true;

#ifdef __arm__
                if ((visualize)&&(isVisualizeActive())) {
                    scrollerPause(); // we need to re-activate too - save state!!!
                    lastVolume = -1;
                    lastBits[0] = 0;
                }
                else
                {
#endif

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
                    putVolume((0 != actVolume), buff);
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
                audio = 3;
                if (1 == samplesize) {
                    sprintf(buff, "DSD%.0f", (samplerate / 44.1));
                    audio++;
                }
                else
                {
                    sprintf(buff, "%db/%.1fkHz", samplesize, samplerate);
                    if (16 != samplesize)
                        audio--;
                }

                if (strstr(buff, ".0") != NULL) {
                    char *foo = NULL;
                    foo = replaceStr(buff, ".0", ""); // cleanup a little

                    sprintf(buff, "%s", foo);
                }

                if (strcmp(lastBits, buff) != 0) {
#ifdef __arm__
                    putAudio(audio, buff);
#endif
                    strncpy(lastBits, buff, 32);
                }

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
                    2, 51, maxXPixel()-4, 4, (pTime * 100) / (dTime == 0 ? 1 : dTime));
#else
                sprintf(buff, "%3ld:%02ld  %5s  %3ld:%02ld", pTime / 60,
                        pTime % 60,
                        (tags[MODE].valid) ? tags[MODE].tagData : "",
                        dTime / 60, dTime % 60);
                sprintf(stbl, "%s\n\n", buff);
                tOut(stbl);
#endif

#ifdef __arm__
                }
#endif
                
                for (int i = 0; i < MAXTAG_TYPES; i++) {
                    tags[i].changed = false;
                }

#ifdef __arm__
                refreshDisplay();
#endif
            } else {

#ifdef __arm__
                scrollerPause(); // we need to re-activate too - save state!!!
#endif

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
                        2, 48, maxXPixel()-4, 4,
                        (int)((100 *
                               (loctm.tm_sec + ((double)tv.tv_usec / 1000000)) /
                               60)));
                    // date
                    sprintf(buff, "  %d-%02d-%02d  ", loctm.tm_year + 1900,
                            loctm.tm_mon + 1, loctm.tm_mday);
                    putTextToCenter(54, buff);

                    // set changed so we'll repaint on play
                    setupPlayMode();
                    lastVolume = -1;
                    lastBits[0] = 0;
                    refreshDisplay();
#endif
                }
            }

            askRefresh();

        } // isRefreshed

#ifdef __arm__
        if ((visualize)&&(isVisualizeActive()))
            refreshDisplay();
        else
            refreshDisplayScroller();
#endif

#ifdef __arm__
//setSnapOff();
#endif
        usleep(SLEEP_TIME);

    } // main loop

    before_exit();
    return 0;
}
