/*
 *	lmsmonitor.c
 *
 *	(c) 2015 László TÓTH
 *	(c) 2020 Stuart Hunter
 *
 *	TODO:
 *          Done - Automatic server discovery
 *          Done - Get playerID automatically
 *          Reconnect to server
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
#include <signal.h>
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
#ifdef SSE_VIZDATA
#include "../sse/vizsse.h"
#endif
#include "vizshmem.h"
#include "visualize.h"
#include "timer.h"
#include "metrics.h"
#include "astral.h"
#include "lmsmonitor.h"
// clang-format on

#endif

#define SLEEP_TIME_SAVER 20
#define SLEEP_TIME_SHORT 80
#define SLEEP_TIME_LONG 160
#define CHRPIXEL 8
#define LINE_NUM 5

char stbl[BSIZE];
tag *tags;
bool playing = false;
bool softlySoftly = false;

// clang-format off
tagtypes_t layout[LINE_NUM][3] = {
	{MAXTAG_TYPES, MAXTAG_TYPES, MAXTAG_TYPES},
	{COMPOSER,     ARTIST,       MAXTAG_TYPES},
	{ALBUM,        MAXTAG_TYPES, MAXTAG_TYPES},
	{TITLE,        MAXTAG_TYPES, MAXTAG_TYPES},
	{ALBUMARTIST,  CONDUCTOR,    MAXTAG_TYPES},
};
// clang-format on

bool freed = false;
// free and cleanup here
void before_exit(void) {
    printf("\nCleanup and shutdown\n");
    if (!freed) { // ??? race condition
        freed = true;
        closeSliminfo();
#ifdef __arm__
        //scrollerPause(); // stop any scrolling prior to clear - no orphans!
        clearDisplay();
        closeDisplay();
#ifdef SSE_VIZDATA
        vissySSEFinalize();
#endif
        vissySHMEMFinalize();
        timer_finalize(); // should be all set ???
#endif
    }
    printf("All Done\nBye Bye.\n");
}

void sigint_handler(int sig) {
    before_exit();
    exit(0);
}

void attach_signal_handler(void) {

    struct sigaction new_action, old_action;

    new_action.sa_handler = sigint_handler;
    sigemptyset(&new_action.sa_mask);
    new_action.sa_flags = 0;

    sigaction(SIGINT, NULL, &old_action);
    if (old_action.sa_handler != SIG_IGN)
        sigaction(SIGINT, &new_action, NULL);
    sigaction(SIGHUP, NULL, &old_action);
    if (old_action.sa_handler != SIG_IGN)
        sigaction(SIGHUP, &new_action, NULL);
    sigaction(SIGTERM, NULL, &old_action);
    if (old_action.sa_handler != SIG_IGN)
        sigaction(SIGTERM, &new_action, NULL);
    sigaction(SIGQUIT, NULL, &old_action);
    if (old_action.sa_handler != SIG_IGN)
        sigaction(SIGQUIT, &new_action, NULL);
    sigaction(SIGSTOP, NULL, &old_action);
    if (old_action.sa_handler != SIG_IGN)
        sigaction(SIGSTOP, &new_action, NULL);

    new_action.sa_flags = SA_NODEFER;
    sigaction(SIGSEGV, &new_action,
              NULL); /* catch seg fault - and hopefully backtrace */
}

// "page" definitions, no break-out for visualizer
#ifdef __arm__
void playingPage(MonitorAttrs *lmsopt);
void nagSaverPage(MonitorAttrs *lmsopt);
void clockPage(MonitorAttrs *lmsopt);
#else
void playingPage(void);
#endif

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
    char buff[128] = {0};
    sprintf(buff,"%s",rindex(executable,'/'));
    if (!isEmptyStr(buff))
        memmove(buff, buff+1, strlen(buff));
    else
        strcpy(buff, executable);
    printf("This is %s (%s) - built %s %s.\n", buff, VERSION, __DATE__, __TIME__);
}

void print_help(char *executable) {
    printf("%s Ver. %s\n"
           "Usage -n \"player name\" [options]\n\n"
           "options:\n"
           " -a all-in-one. One screen to rule them all. Track and visualizer on one screen (pi only)\n"
           " -b automatically set brightness of display at sunset and sunrise (pi only)\n"
           " -c display clock when not playing (Pi only)\n"
           " -d downmix audio and display a single large meter, SA and VU only\n"
           " -i increment verbose level\n"
           " -k show CPU temperature (clock mode)\n"
           " -m if visualization on specify one or more meter modes, sa, vu, "
           "pk, or rn for random\n"
           " -o specified OLED \"driver\" type (see options below)\n"
           " -r show remaining time rather than track time\n"
           " -t enable print info to stdout\n"
           " -v enable visualization sequence when playing (Pi only)\n"
           " -x specify OLED address if default does not work - use i2cdetect to find address (Pi only)\n"
           " -z no splash screen\n\n",
           APPNAME, VERSION);
#ifdef __arm__
    printOledTypes();
#endif
    signature(executable);
}

#ifdef __arm__

void setupPlayMode(void) {
    for (int line = 1; line < LINE_NUM; line++) {
        for (tagtypes_t *t = layout[line]; *t != MAXTAG_TYPES; t++) {
            if (tags[*t].valid) {
                tags[*t].changed = true;
            }
        }
    }
}

bool isPlaying(void) { return playing; }

void toggleVisualize(size_t timer_id, void *user_data) {

    MonitorAttrs *lmsopt = user_data;
    if (lmsopt->visualize) {
        if (playing) {
            if (isVisualizeActive()) {
                deactivateVisualizer();
                softlySoftly = true;
            } else {
                resetDisplay(1);
                activateVisualizer();
            }
            clearDisplay();
        }
    }
}

void checkAstral(size_t timer_id, void *user_data) { brightnessEvent(); }

void cycleVisualize(size_t timer_id, void *user_data) {

    MonitorAttrs *lmsopt = user_data;
    if (lmsopt->visualize) {
        if (playing) {
            if (isVisualizeActive()) 
            {
                vis_type_t current = {0};
                vis_type_t updated = {0};
                strncpy(current, getVisMode(), 2);
                strncpy(updated, currentMeter(), 2);
                if (strcmp(current, updated) != 0)
                {
                    lmsopt->refreshLMS = true;
                    clearDisplay();
                }
            } else {
                currentMeter();
            }
        } else {
            currentMeter();
        }
    }
}

#endif

int main(int argc, char *argv[]) {

    // direct path for help
    if( argc == 2 )
    {
        if ((strncmp(argv[1],"-h",2) == 0) ||
            (strncmp(argv[1],"--h",3) == 0)) {
                print_help(argv[0]);
                exit(EXIT_SUCCESS);
        }
    }

    // require elevated privs for SHMEM
    // should only do this if we have v param
    if (geteuid() != 0) {
        printf("\nPlease run as root (sudo) to "
        "access shared memory (visualizer)\n\n");
        exit(EXIT_FAILURE);
    }

    attach_signal_handler();

#ifdef __arm__
    struct MonitorAttrs lmsopt = {
        .oledAddrL = 0x3c,
        .oledAddrR = 0x3c,
        .vizHeight = 64,
        .vizWidth = 128,
        .allInOne = false,
        .sleepTime = SLEEP_TIME_LONG,
        .astral = false,
        .showTemp = false,
        .downmix = false,
        .nagDone = false,
        .visualize = false,
        .meterMode = false,
        .clock = false,
        .extended = false,
        .remaining = false,
        .splash = true,
        .refreshLMS = false,
        .refreshClock = false,
        .lastVolume = -1,
    };

    strcpy(lmsopt.lastBits, "LMS");
    strcpy(lmsopt.lastTemp, "00000");
    strcpy(lmsopt.lastTime, "XX:XX"); // non-matching time digits (we compare one by one)!

#endif

    char *playerName = NULL;

    long lastVolume = -10;
    int aName;
    char *thatMAC = NULL;
    char *thisMAC = get_mac_address();

#ifdef __arm__
#ifdef SSE_VIZDATA
    Options opt;
#endif
    srand(time(0));
#endif

    opterr = 0;
    while ((aName = getopt(argc, argv, "n:o:m:x:B:abcdhikprtvz")) != -1) {
        switch (aName) {
            case 'n': playerName = optarg; break;

            case 't': enableTOut(); break;

            case 'i': incVerbose(); break;

#ifdef __arm__
            case 'o':
                int oled;
                sscanf(optarg, "%d", &oled);
                if (oled < 0 || oled >= OLED_LAST_OLED ||
                    !strstr(oled_type_str[oled], "128x64")) {
                    printf("*** invalid 128x64 oled type %d\n", oled);
                    print_help(argv[0]);
                    exit(EXIT_FAILURE);
                } else {
                    setOledType(oled);
                }
                break;
            case 'a':
                lmsopt.allInOne = true;
                break;
            case 'b':
                lmsopt.astral = true;
                break;
            case 'm':
                setVisList(optarg);
                lmsopt.meterMode = true;
                break;
#ifdef SSE_VIZDATA
            case 'p': sscanf(optarg, "%d", &opt.port); break;
#else
            case 'p': printf("SSE not supported\n"); break;
#endif
            case 'v': lmsopt.visualize = true; break;

            case 'c': lmsopt.clock = true; break;

            case 'd': lmsopt.downmix = true; break;

            case 'k': lmsopt.showTemp = true; break;

            case 'r': lmsopt.remaining = true; break;

            case 'x':
                lmsopt.oledAddrL = (int)strtol(optarg, NULL, 16);
                //lmsopt.oledAddrR = (int)strtol(hexstr1, NULL, 16);
                setOledAddress(lmsopt.oledAddrL);
                break;
            case 'z': lmsopt.splash = false; break;

            case 'B': printf("TODO\n"); break;

#endif

            case 'h':
                print_help(argv[0]);
                exit(EXIT_SUCCESS);
                break;
        }
    }

#ifdef __arm__

    if (!(lmsopt.meterMode)) {
        char vinit[3] = {0};
        strcpy(vinit, VMODE_RN); // intermediate - pointers be damned!
        setVisList(vinit);
    }

    // init OLED display
    if (initDisplay() == EXIT_FAILURE) {
        exit(EXIT_FAILURE);
    }
    // throw up splash if not nixed by caller
    if (lmsopt.splash)
        splashScreen();
    else
        displayBrightness(MAX_BRIGHTNESS);

#endif

    signature(argv[0]);
#ifdef __arm__
    printOledSetup();  // feedback and "debug"
#endif
    thatMAC = playerMAC();

    // init here - splash delay mucks the refresh flagging
    if ((tags = initSliminfo(playerName)) == NULL) {
        exit(EXIT_FAILURE);
    }

#ifdef __arm__
    // setup brightness event (sunrise/sunset)
    if ((lmsopt.astral)&&(initAstral())) {
        size_t astraltimer;
        timer_initialize();
        astraltimer = timer_start(60 * 5 * 1000, checkAstral, TIMER_PERIODIC,
                                  (void *)NULL);
    }
#endif

#ifdef __arm__

    // init for visualize mechanism here
    if (strcmp(thatMAC, thisMAC) != 0) {
        if (lmsopt.visualize) {
#ifdef SSE_VIZDATA
            // go SSE for visualizer
            opt.host = getPlayerIP();
            opt.port = 8022;
            opt.endpoint = "/visionon?subscribe=SA-VU";
            lmsopt.visualize = setupSSE(opt);
#else
            printf("SSE not supported, deactivating visualizer\n");
            lmsopt.visualize = false;
#endif
        }
    } else {

        if (lmsopt.visualize) {
            // go SHMEM for visualizer
            lmsopt.visualize = setupSHMEM();
        }
    }

    // if we had a clean setup
    // init visualizer mode and
    // timer toggle

    if (lmsopt.visualize) {
        sayVisList();
        size_t viztimer;
        size_t vizcycletimer;
        timer_initialize();
        viztimer = timer_start(60 * 1000, toggleVisualize, TIMER_PERIODIC,
                               (void *)&lmsopt);
        vizcycletimer = timer_start(99 * 1000, cycleVisualize, TIMER_PERIODIC,
                                    (void *)&lmsopt);

        sprintf(stbl, "%s %s\n", 
            labelIt("Downmix VU+SA", LABEL_WIDTH, "."), 
            ((lmsopt.downmix)?"Yes":"No"));

    } else {
        sprintf(stbl, "%s Inactive\n", 
            labelIt("Visualization", LABEL_WIDTH, ".")); 
    }
    putMSG(stbl, LL_INFO);    

#endif

#ifdef __arm__
    clearDisplay(); // clear splash if shown
    // bitmap capture
#ifdef CAPTURE_BMP
    //setSnapOn();
    //setSnapOff();
#endif
#endif

    while (true) {

#ifdef __arm__
        if (softlySoftly) {
            softlySoftly = false;
            lmsopt.refreshLMS = true;
            lmsopt.refreshClock = true;
            clearDisplay();
        }
#endif

        if (isRefreshed()) {

            playing = (strcmp("play", tags[MODE].tagData) == 0);

            if (playing) {

#ifdef __arm__
                if (!isVisualizeActive())
                    playingPage(&lmsopt);
                else
                    setupPlayMode();
#else
                playingPage();
#endif
            } else {
#ifdef __arm__
                //scrollerPause(); // we need to re-activate too - save state!!!
                if (lmsopt.clock)
                    clockPage(&lmsopt);
                else
                    nagSaverPage(&lmsopt);
#endif
            } // playing

            askRefresh();

        } // isRefreshed

#ifdef __arm__
        if (lmsopt.nagDone)
            refreshDisplay();
#endif

#ifdef __arm__
//setSnapOff();
#endif

#ifdef __arm__
        if (lmsopt.sleepTime < 1)
            lmsopt.sleepTime = SLEEP_TIME_LONG;
        dodelay(lmsopt.sleepTime);
#else
        dodelay(SLEEP_TIME_LONG);
#endif

    } // main loop

    before_exit();
    return 0;
}

#ifdef __arm__

bool nagSetup = true;
void nagSaverPage(MonitorAttrs *lmsopt) {

    // musical notes floater
    if (!lmsopt->nagDone) {
        if (nagSetup)
            nagSaverSetup();
        nagSetup = false;
        nagSaverNotes();
        lmsopt->sleepTime = SLEEP_TIME_SAVER;
        lmsopt->refreshLMS = true;
    }
}
#endif

#ifdef __arm__
void clockPage(MonitorAttrs *lmsopt) {

    char buff[255];

    if (lmsopt->refreshClock) {
        resetDisplay(1);
        strncpy(lmsopt->lastTime, "XX:XX", 6); // force, just in case
        strncpy(lmsopt->lastTemp, "00000", 6); // force
        lmsopt->refreshClock = false;
    }
    lmsopt->refreshLMS = true;
    lmsopt->nagDone = true;

    lmsopt->sleepTime = SLEEP_TIME_LONG;

    if (lmsopt->showTemp)
    {
        sprintf(buff, "%.1fC", cpuTemp());
        if (strcmp(lmsopt->lastTemp, buff) != 0) {
            int y = 39;
            if (strcmp(lmsopt->lastTemp, "00000") == 0) {
                putTextToCenter(y, buff);
            }
            else {
                putTextCenterColor(y, lmsopt->lastTemp, BLACK);
                putTextCenterColor(y, buff, WHITE);
            }
            strncpy(lmsopt->lastTemp, buff, 9);
        }
    }

    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t now = tv.tv_sec;
    struct tm loctm = *localtime(&now);

    // time
    sprintf(buff, "%02d:%02d", loctm.tm_hour, loctm.tm_min);
    if (strcmp(lmsopt->lastTime, buff) != 0) {
        drawTimeText2(buff, lmsopt->lastTime);
        strncpy(lmsopt->lastTime, buff, 32);
    }

    // colon (blink)
    if (loctm.tm_sec % 2)
        drawTimeBlink(' ');
    else
        drawTimeBlink(':');

    // seconds
    drawHorizontalBargraph(
        2, 48, maxXPixel() - 4, 4,
        (int)((100 * (loctm.tm_sec + ((double)tv.tv_usec / 1000000)) / 60)));
    // date
    strftime(buff, sizeof(buff), "%A %Y-%m-%02d", &loctm);
    putTextToCenter(54, buff);

    // set changed so we'll repaint on play
    setupPlayMode();
    lmsopt->lastVolume = -1;
    lmsopt->lastBits[0] = 0;
    refreshDisplay();
}
#endif

#ifdef __arm__
void playingPage(MonitorAttrs *lmsopt)
#else
void playingPage(void)
#endif
{

    long actVolume = 0;
    int audio = 3; // 2 HD 3 SD 4 DSD
    long pTime, dTime, rTime;
    char buff[255];

#ifdef __arm__
    if (lmsopt->refreshLMS) {
        resetDisplay(1);
        lmsopt->refreshLMS = false;
    }
    lmsopt->refreshClock = true;
    lmsopt->nagDone = true;

#endif

#ifdef __arm__

    // output sample rate and bit depth too, supports DSD nomenclature
    double samplerate = tags[SAMPLERATE].valid
                            ? strtof(tags[SAMPLERATE].tagData, NULL) / 1000
                            : 44.1;
    int samplesize = tags[SAMPLESIZE].valid
                         ? strtol(tags[SAMPLESIZE].tagData, NULL, 10)
                         : 16;

    if (lmsopt->downmix)
        setDownmix(samplesize, samplerate);
    else
        setDownmix(0, 0);

    if ((lmsopt->visualize) && (isVisualizeActive())) {
        lmsopt->lastVolume = -1;
        lmsopt->lastBits[0] = 0;
        lmsopt->sleepTime = SLEEP_TIME_SHORT;
    } else {
#endif
        sprintf(buff, "________________________\n");
        tOut(buff);

#ifdef __arm__
        actVolume =
            tags[VOLUME].valid ? strtol(tags[VOLUME].tagData, NULL, 10) : 0;
        if (actVolume != lmsopt->lastVolume) {
            if (0 == actVolume)
                strncpy(buff, "  mute", 32);
            else
                sprintf(buff, "  %ld%% ", actVolume);
            putVolume((0 != actVolume), buff);
            lmsopt->lastVolume = actVolume;
        }

        audio = 3;
        if (1 == samplesize) {
            sprintf(buff, "DSD%.0f", (samplerate / 44.1));
            audio++;
        } else {
            sprintf(buff, "%db/%.1fkHz", samplesize, samplerate);
            if (16 != samplesize)
                audio--;
        }

        if (strstr(buff, ".0") != NULL) {
            char *foo = NULL;
            foo = replaceStr(buff, ".0", ""); // cleanup a little
            sprintf(buff, "%s", foo);
            if (foo)
                free(foo);
        }

        if (strcmp(lmsopt->lastBits, buff) != 0) {
            putAudio(audio, buff);
            strncpy(lmsopt->lastBits, buff, 32);
        }

#endif

        for (int line = 1; line < LINE_NUM; line++) {
#ifdef __arm__
            bool filled = false;
#endif
            for (tagtypes_t *t = layout[line]; *t != MAXTAG_TYPES; t++) {

                if (tags[*t].valid) {
#ifdef __arm__
                    filled = true;
#endif
                    if (tags[*t].changed) {
                        strncpy(buff, tags[*t].tagData, 255);
#ifdef __arm__
                        if (putScrollable(line, buff)) {
                            lmsopt->sleepTime = SLEEP_TIME_SHORT;
                        }
#endif
                    }
                    sprintf(stbl, "%s\n", tags[*t].tagData);
                    tOut(stbl);
                    break;
                }
            }
#ifdef __arm__
            if (!filled) {
                clearScrollable(line);
                clearLine(line * 10);
            }
#endif
        }

#ifdef __arm__
        lmsopt->sleepTime = SLEEP_TIME_SHORT;
#endif

        pTime = (tags[TIME].valid) ? strtol(tags[TIME].tagData, NULL, 10) : 0;
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
        if (lmsopt->remaining)
            sprintf(buff, "-%02ld:%02ld", rTime / 60, rTime % 60);
        else
            sprintf(buff, "%02ld:%02ld", dTime / 60, dTime % 60);

        int dlen = strlen(buff);
        putText(maxXPixel() - (dlen * CHAR_WIDTH) - 1, 56, buff);

        sprintf(buff, "%s", (tags[MODE].valid) ? tags[MODE].tagData : "");
        int mlen = strlen(buff);
        putText(((maxXPixel() - ((tlen + mlen + dlen) * CHAR_WIDTH)) / 2) +
                    (tlen * CHAR_WIDTH),
                56, buff);

        drawHorizontalBargraph(2, 51, maxXPixel() - 4, 4,
                               (pTime * 100) / (dTime == 0 ? 1 : dTime));
#endif

    sprintf(buff, "%3ld:%02ld  %5s  %3ld:%02ld", pTime / 60, pTime % 60,
            (tags[MODE].valid) ? tags[MODE].tagData : "", dTime / 60,
            dTime % 60);
    sprintf(stbl, "%s\n\n", buff);
    tOut(stbl);

#ifdef __arm__
    }
#endif
    for (int i = 0; i < MAXTAG_TYPES; i++) {
        tags[i].changed = false;
    }

#ifdef __arm__
    refreshDisplay();
#endif
}
