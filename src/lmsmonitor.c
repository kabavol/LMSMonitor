/*
 *	lmsmonitor.c
 *
 *	(c) 2015 László TÓTH
 *	(c) 2020 Stuart Hunter
 *
 *	TODO:
 *          DONE - Automatic server discovery
 *          DONE - Get playerID automatically
 *          Reconnect to server (after bounce)
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
#include "cnnxn.h"
#include "lmsmonitor.h"
#include "oledimg.h"
// clang-format on
struct MonitorAttrs *glopt;

#endif

#define SLEEP_TIME_SAVER 20
#define SLEEP_TIME_SHORT 80
#define SLEEP_TIME_LONG 160
#define CHRPIXEL 8
#define LINE_NUM 5
#define A1LINE_NUM 2
#define A1SCROLLER 5

char stbl[BSIZE];
char remTime[9];
tag *tags;
//bool playing = false;
#define playing isTrackPlaying()
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
        scrollerPause();
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
void playingPage(void);
#ifdef __arm__
void saverPage(void);
void clockPage(void);
void cassettePage(A1Attributes *aio);
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
    sprintf(buff, "%s", rindex(executable, '/'));
    if (!isEmptyStr(buff))
        memmove(buff, buff + 1, strlen(buff));
    else
        strcpy(buff, executable);
    printf("This is %s (%s) - built %s %s.\n", buff, VERSION, __DATE__,
           __TIME__);
}

void print_help(char *executable) {
    printf(
        "%s Ver. %s\n"
        "Usage -n \"player name\" [options]\n\n"
        "options:\n"
        " -a all-in-one. One screen to rule them all. Track and visualizer on "
        "one screen (pi only)\n"
        " -b automatically set brightness of display at sunset and sunrise"
        " (pi only)\n"
        " -c display clock when not playing (Pi only)\n"
        " -d downmix audio and display a single large meter, SA and VU only\n"
        " -f font used by clock, see list below for details\n"
        " -i increment verbose level\n"
        " -I flip the display - display mounted upside down\n"
        " -k show CPU load and temperature (clock mode)\n"
        " -m if visualization on specify one or more meter modes, sa, vu, "
        "pk, st, or rn for random\n"
        " -o specifies OLED \"driver\" type (see options below)\n"
        " -r show remaining time rather than track time\n"
        " -S scrollermode: 0 (cylon), 1 (infinity left), 2 infinity (right)\n"
        " -v enable visualization sequence when playing (Pi only)\n"
        " -x specifies OLED address if default does not work - use i2cdetect "
        "to find address (Pi only)\n"
        " -B I2C bus number (defaults 1, giving device /dev/i2c-1)\n"
        " -R I2C/SPI reset GPIO number, if needed (defaults 25)\n"
        " -D SPI DC GPIO number (defaults 24)\n"
        " -C SPI CS number (defaults 0)\n"
        " -z no splash screen\n\n",
        APPNAME, VERSION);
#ifdef __arm__
    printOledTypes();
    printOledFontTypes();
#endif
    signature(executable);
}

bool isTrackPlaying(void) {
    bool p = false;
    p = (strcmp("play", tags[MODE].tagData) == 0);
#ifdef __arm__
    setPlaying(p);
#endif
    return p;
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

bool acquireOptLock(void) {

    char buff[128] = {0};
    bool ret = true;
    if (glopt) {
        uint8_t test = 0;
        while (pthread_mutex_trylock(&glopt->update) != 0) {
            if (test > 30) {
                ret = false;
                strcpy(buff, "options mutex acquire failed\n");
                putMSG(buff, LL_DEBUG);
                break;
            }
            usleep(10);
            test++;
        }
    }
    return ret;
}

void softClockRefresh(bool rc = true) {
    if (rc != glopt->refreshClock)
        if (acquireOptLock()) {
            glopt->refreshClock = rc;
            pthread_mutex_unlock(&glopt->update);
        }
}

void softVisualizeRefresh(bool rv = true) {
    if (glopt->visualize)
        if (rv != glopt->refreshViz)
            if (acquireOptLock()) {
                glopt->refreshViz = rv;
                pthread_mutex_unlock(&glopt->update);
            }
}

void softPlayRefresh(bool r = true) {
    if (r != glopt->refreshLMS)
        if (acquireOptLock()) {
            glopt->refreshLMS = r;
            pthread_mutex_unlock(&glopt->update);
        }
}

void setLastTime(char *tm, DrawTime dt) {
    if (strcmp(glopt->lastTime, tm) != 0) {
        if (acquireOptLock()) {
            drawTimeText(tm, glopt->lastTime, &dt);
            strncpy(glopt->lastTime, tm, 32);
            pthread_mutex_unlock(&glopt->update);
        }
    }
}

void setLastRemainingTime(char *rtm, DrawTime rdt) {
    if (strcmp(remTime, rtm) != 0) {
        drawRemTimeText(rtm, remTime, &rdt);
        drawTimeBlink(':', &rdt);
        strncpy(remTime, rtm, 9);
    }
}

void setSleepTime(int st) {
    if (st != glopt->sleepTime)
        if (acquireOptLock()) {
            glopt->sleepTime = st;
            pthread_mutex_unlock(&glopt->update);
        }
}

void putCPUMetrics(int y) {
    if (acquireOptLock()) {
        sprintf(glopt->lastLoad, "%.0f%%", cpuLoad());
        sprintf(glopt->lastTemp, "%.1fC", cpuTemp());
        char buff[BSIZE] = {0};
        sprintf(buff, "%s %s", glopt->lastLoad, glopt->lastTemp);
        putTextCenterColor(y, buff, WHITE);
        pthread_mutex_unlock(&glopt->update);
    }
}

void softClockReset(bool cd = true) {
    if (acquireOptLock()) {
        strcpy(glopt->lastTime, "XX:XX");
        strcpy(glopt->lastTemp, "00000");
        strcpy(glopt->lastLoad, "00000");
        glopt->refreshClock = true;
        pthread_mutex_unlock(&glopt->update);
        if (cd)
            clearDisplay();
    }
}

void setLastVolume(int vol) {
    if (vol != glopt->lastVolume)
        if (acquireOptLock()) {
            glopt->lastVolume = vol;
            pthread_mutex_unlock(&glopt->update);
        }
}

void setLastBits(char *lb) {
    if (strcmp(lb, glopt->lastBits) != 0)
        if (acquireOptLock()) {
            strncpy(glopt->lastBits, lb, 32);
            pthread_mutex_unlock(&glopt->update);
        }
}

void setLastModes(int8_t lm[]) {
    if ((lm[SHUFFLE_BUCKET] != glopt->lastModes[SHUFFLE_BUCKET]) ||
        (lm[REPEAT_BUCKET] != glopt->lastModes[REPEAT_BUCKET])) {
        if (acquireOptLock()) {
            glopt->lastModes[SHUFFLE_BUCKET] = lm[SHUFFLE_BUCKET];
            glopt->lastModes[REPEAT_BUCKET] = lm[REPEAT_BUCKET];
            pthread_mutex_unlock(&glopt->update);
        }
    }
}

void softPlayReset(void) {
    if (acquireOptLock()) {
        glopt->lastBits[0] = 0;
        glopt->lastVolume = -1;
        glopt->refreshLMS = true;
        pthread_mutex_unlock(&glopt->update);
    }
}

void setNagDone(bool refresh = true) {
    if (acquireOptLock()) {
        if (refresh)
            glopt->refreshLMS = true;
        glopt->nagDone = true;
        pthread_mutex_unlock(&glopt->update);
    }
}

void toggleVisualize(size_t timer_id, void *user_data) {

    instrument(__LINE__, __FILE__, "toggleVisualize");

    if (glopt->visualize) {
        if (playing) {
            instrument(__LINE__, __FILE__, "test Visualize");
            if (isVisualizeActive()) {
                instrument(__LINE__, __FILE__, "deactivate Visualize");
                deactivateVisualizer();
            } else {
                instrument(__LINE__, __FILE__, "scroller pause");
                if (activeScroller()) {
                    scrollerPause();
                }
                instrument(__LINE__, __FILE__, "activate Visualize");
                activateVisualizer();
            }
            setupPlayMode();
            softlySoftly = true;
            softVisualizeRefresh(true);
        }
    }
}

void hubSpin(size_t timer_id, void *user_data) {

    A1Attributes *aio;
    aio = ((struct A1Attributes *)user_data);
    if (aio->hubActive) {
        aio->lFrame += aio->flows; // clockwise (-1) or anticlockwise (1)
        if (aio->lFrame < 0)
            aio->lFrame = aio->mxframe; // roll
        if (aio->lFrame > aio->mxframe)
            aio->lFrame = 0; // roll
        aio->rFrame += aio->flows;
        if (aio->rFrame < 0)
            aio->rFrame = aio->mxframe;
        if (aio->rFrame > aio->mxframe)
            aio->rFrame = 0;
        cassetteHub(38, aio->lFrame, aio->mxframe, -aio->flows);
        cassetteHub(80, aio->rFrame, aio->mxframe, -aio->flows);
    }
}

void checkAstral(size_t timer_id, void *user_data) { brightnessEvent(); }

void cycleVisualize(size_t timer_id, void *user_data) {

    instrument(__LINE__, __FILE__, "cycleVisualize");
    if (glopt->visualize) {
        instrument(__LINE__, __FILE__, "Visualize Is On");
        if (playing) {
            instrument(__LINE__, __FILE__, "isPlaying");
            if (isVisualizeActive()) {
                instrument(__LINE__, __FILE__, "Active Visualization");
                vis_type_t current = {0};
                vis_type_t updated = {0};
                strncpy(current, getVisMode(), 3);
                strncpy(updated, currentMeter(), 3);
                if (strcmp(current, updated) != 0) {
                    softlySoftly = true;
                    if (0 == strncmp(VMODE_A1S, updated, 3))
                        setA1Downmix(VEMODE_A1S);
                    else if (0 == strncmp(VMODE_A1V, updated, 3))
                        setA1Downmix(VEMODE_A1V);
                }
                setInitRefresh();
            } else {
                currentMeter();
            }
            softVisualizeRefresh(true);
        } else {
            currentMeter();
        }
    }
}

void sampleDetails(audio_t *audio) {
    audio->samplerate = tags[SAMPLERATE].valid
                            ? strtof(tags[SAMPLERATE].tagData, NULL) / 1000
                            : 44.1;
    audio->samplesize = tags[SAMPLESIZE].valid
                            ? strtol(tags[SAMPLESIZE].tagData, NULL, 10)
                            : 16;
    audio->volume =
        tags[VOLUME].valid ? strtol(tags[VOLUME].tagData, NULL, 10) : 0;
    audio->repeat =
        tags[REPEAT].valid ? strtol(tags[REPEAT].tagData, NULL, 10) : 0;
    audio->shuffle =
        tags[SHUFFLE].valid ? strtol(tags[SHUFFLE].tagData, NULL, 10) : 0;
}

#endif

int main(int argc, char *argv[]) {

    // direct path for help
    if (argc == 2) {
        if ((strncmp(argv[1], "-h", 2) == 0) ||
            (strncmp(argv[1], "--h", 3) == 0)) {
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
    instrument(__LINE__, __FILE__, "Initialize");

    struct MonitorAttrs lmsopt = {
        .oledAddrL = 0x3c,
        .oledAddrR = 0x3c,
        .vizHeight = 64,
        .vizWidth = 128,
        .allInOne = false,
        .tapeUX = false,
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
        .refreshLMS = true, // fresh up!
        .refreshClock = false,
        .refreshViz = false,
        .lastVolume = -1,
        .flipDisplay = false,
        .i2cBus = 1,               // number of I2C bus
        .oledRST = OLED_SPI_RESET, // SPI/IIC reset GPIO
        .spiDC = OLED_SPI_DC,      // SPI DC
        .spiCS = BCM2835_SPI_CS0,  // SPI CS - 0: CS0, 1: CS1
        .lastModes = {0, 0},
    };

    if (pthread_mutex_init(&lmsopt.update, NULL) != 0) {
        closeDisplay();
        printf("\nLMS Options mutex init has failed\n");
        exit(EXIT_FAILURE);
    }

    strcpy(lmsopt.lastBits, "LMS");
    strcpy(lmsopt.lastTemp, "00000");
    strcpy(lmsopt.lastTime, "XX:XX");
    lmsopt.clockFont = MON_FONT_CLASSIC;
    glopt = &lmsopt;
    strcpy(remTime, "XXXXX");

#endif

    char *playerName = NULL;
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
    int a1test = 0;
    while ((aName = getopt(argc, argv, "n:o:m:x:B:C:D:R:S:f:abcdFhikprtuvz")) !=
           -1) {
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
                lmsopt.downmix = true;
                lmsopt.visualize = true;
                a1test++;
                break;
            case 'b': lmsopt.astral = true; break;
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

            case 'f':
                if (optarg) {
                    int oled;
                    sscanf(optarg, "%d", &oled);
                    if (oled < MON_FONT_CLASSIC || oled >= MON_FONT_LCD1521) {
                        printf("*** invalid oled font %d\n", oled);
                        print_help(argv[0]);
                        exit(EXIT_FAILURE);
                    } else {
                        lmsopt.clockFont = oled;
                    }
                }
                break;

            case 'F': lmsopt.flipDisplay = true; break;

            case 'k': lmsopt.showTemp = true; break;

            case 'r': lmsopt.remaining = true; break;

            case 'u': lmsopt.tapeUX = true; break;

            case 'x':
                lmsopt.oledAddrL = (int)strtol(optarg, NULL, 16);
                //lmsopt.oledAddrR = (int)strtol(hexstr1, NULL, 16);
                setOledAddress(lmsopt.oledAddrL);
                break;
            case 'z': lmsopt.splash = false; break;

            case 'S':
                int sm;
                sscanf(optarg, "%d", &sm);
                setScrollMode(sm);
                break;
            case 'B':
                int i2b;
                sscanf(optarg, "%d", &i2b);
                lmsopt.i2cBus = ((i2b == 0) || (i2b == 1)) ? i2b : 1;
                break;
            case 'C':
                int sCS;
                sscanf(optarg, "%d", &sCS);
                lmsopt.spiCS =
                    ((sCS == BCM2835_SPI_CS0) || (sCS == BCM2835_SPI_CS1))
                        ? sCS
                        : BCM2835_SPI_CS0;
                break;
            case 'D':
                int sDC;
                sscanf(optarg, "%d", &sDC);
                //>0 < 27 != RST
                lmsopt.spiDC = sDC;
                break;
            case 'R':
                int sRST;
                sscanf(optarg, "%d", &sRST);
                //>0 < 27 != DC
                lmsopt.oledRST = sRST;
                break;

#endif

            case 'h':
                print_help(argv[0]);
                exit(EXIT_SUCCESS);
                break;
        }
    }

#ifdef __arm__

    if (lmsopt.allInOne) {
        lmsopt.meterMode = true;
        lmsopt.downmix = true;   // safe
        lmsopt.visualize = true; // safe
        setA1VisList();
        instrument(__LINE__, __FILE__, "allInOne setup");
    } else {
        if (!(lmsopt.meterMode)) {
            char vinit[3] = {0};
            strcpy(vinit, VMODE_RN); // intermediate - pointers be damned!
            setVisList(vinit);
        }
    }

    // init OLED display IIC & SPI supported
    if (initDisplay(lmsopt) == EXIT_FAILURE) {
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
    sprintf(stbl, "%s %s\n", labelIt("Verbosity", LABEL_WIDTH, "."),
            getVerboseStr());
    putMSG(stbl, LL_QUIET);
    printOledSetup();    // feedback and "debug"
    printScrollerMode(); // feedback and "debug"
    sprintf(stbl, "%s %s\n", labelIt("OLED Clock Font", LABEL_WIDTH, "."),
            oled_font_str[lmsopt.clockFont]);
    putMSG(stbl, LL_QUIET);
#endif
    thatMAC = playerMAC();

    // init here - splash delay mucks the refresh flagging
    if ((tags = initSliminfo(playerName)) == NULL) {
        exit(EXIT_FAILURE);
    }

#ifdef __arm__
    // setup brightness event (sunrise/sunset)
    if ((lmsopt.astral) && (initAstral())) {
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

    A1Attributes aio = {
        .hubActive = false,
        .lFrame = 1,  // left hub
        .rFrame = 4,  // right hub
        .mxframe = 6, // max frames
        .flows = 1,   // clockwise (-1) or anticlockwise (1)
        .artist = {0},
        .title = {0},
        .compound = {0},
    };

    // if we had a clean setup
    // init visualizer mode and
    // timer toggle

    if (lmsopt.visualize) {
        sayVisList();
        size_t viztimer;
        size_t vizcycletimer;
        timer_initialize();

        // if the A1 factor is normal then cycle - else fixed visualize

        if (a1test <= 1) {
            viztimer = timer_start(120 * 1000, toggleVisualize, TIMER_PERIODIC,
                                   (void *)NULL);
        } else {
            activateVisualizer(); // fixed all-in-one (when playing)
        }

        if (!(lmsopt.allInOne)) {
            instrument(__LINE__, __FILE__, "activate visualization cycling");
            vizcycletimer = timer_start(254 * 1000, cycleVisualize,
                                        TIMER_PERIODIC, (void *)NULL);
            sprintf(stbl, "%s %s\n", labelIt("Downmix VU+SA", LABEL_WIDTH, "."),
                    ((lmsopt.downmix) ? "Yes" : "No"));
        } else {
            // faster cycle for A1 - unless A1 factor high
            int r = ((1 == a1test) ? 127 : 254);
            instrument(__LINE__, __FILE__,
                       "activate All-In-One visualization cycling");
            vizcycletimer = timer_start(r * 1000, cycleVisualize,
                                        TIMER_PERIODIC, (void *)NULL);
            sprintf(stbl, "%s %s\n",
                    labelIt("Downmix A1 (VU+SA)", LABEL_WIDTH, "."),
                    ((lmsopt.downmix) ? "Yes" : "No"));
        }

    } else {
        sprintf(stbl, "%s Inactive\n",
                labelIt("Visualization", LABEL_WIDTH, "."));
        if (lmsopt.tapeUX) {
            size_t hubtimer;
            hubtimer = timer_start(200, hubSpin, TIMER_PERIODIC, (void *)&aio);
        }
    }
    putMSG(stbl, LL_INFO);

    clearDisplay(); // clears the  splash if shown
    showConnect(); // connection helper
    clearDisplay();

    printFontMetrics();

// bitmap capture
#ifdef CAPTURE_BMP
//setSnapOn();
//setSnapOff();
#endif
#endif

    while (true) {

        if (isRefreshed()) {

#ifdef __arm__
            instrument(__LINE__, __FILE__, "isRefreshed");
            if (softlySoftly) {
                softlySoftly = false;
                softPlayReset();
                softClockReset(false);
                instrument(__LINE__, __FILE__, "Softly Softly <-");
                clearDisplay(); // refreshDisplay();
            }

#endif
            if (isTrackPlaying()) {

#ifdef __arm__

                instrument(__LINE__, __FILE__, "isPlaying");

                // Threaded logic in play - DO NOT MODIFY
                if (!isVisualizeActive()) {
                    if (!lmsopt.tapeUX) {
                        clearScrollable(A1SCROLLER);
                        playingPage();
                    } else {
                        cassettePage(&aio);
                    }
                    strcpy(remTime, "XXXXX");
                } else {
                    if (strncmp(getVisMode(), VMODE_A1, 2) == 0) {
                        allInOnePage(&aio);
                    }
                }
                // Threaded logic in play - DO NOT MODIFY
#else
                playingPage();
#endif
            } else {
#ifdef __arm__
                instrument(__LINE__, __FILE__, "activeScroller test");
                if (activeScroller()) {
                    scrollerPause();
                }
                strcpy(remTime, "XXXXX");
                instrument(__LINE__, __FILE__, "display clock test");
                if (lmsopt.clock) {
                    if ((lmsopt.tapeUX) && (aio.hubActive))
                        aio.hubActive = false;
                    clockPage();
                } else {
                    saverPage();
                }
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
            setSleepTime(SLEEP_TIME_LONG);
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
void saverPage(void) {

    // musical notes floater
    // only show this once at startup (non-clock)
    if (!glopt->nagDone) {
        if (nagSetup)
            nagSaverSetup();
        nagSetup = false;
        nagSaverNotes();
        setSleepTime(SLEEP_TIME_SAVER);
        softPlayReset();
    }
}
#endif

#ifdef __arm__
void clockPage(void) {

    char buff[255];

    DrawTime dt = {.charWidth = 25,
                   .charHeight = 44,
                   .bufferLen = LCD25X44_LEN,
                   .pos = {2, ((glopt->showTemp) ? -1 : 1)},
                   .font = glopt->clockFont};

    if (glopt->refreshClock) {
        instrument(__LINE__, __FILE__, "clock reset");
        softClockReset();
        softClockRefresh(false);
    }
    instrument(__LINE__, __FILE__, "clockPage");

    setNagDone();
    softPlayReset();
    softVisualizeRefresh(true);
    setSleepTime(SLEEP_TIME_LONG);

    instrument(__LINE__, __FILE__, "cpu Metrics?");
    if (glopt->showTemp)
        putCPUMetrics(39);

    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t now = tv.tv_sec;
    struct tm loctm = *localtime(&now);

    // time
    sprintf(buff, "%02d:%02d", loctm.tm_hour, loctm.tm_min);
    if (strcmp(glopt->lastTime, buff) != 0)
        setLastTime(buff, dt);

    // colon (blink)
    drawTimeBlink(((loctm.tm_sec % 2) ? ' ' : ':'), &dt);

    // seconds
    drawHorizontalBargraph(
        2, 48, maxXPixel() - 4, 4,
        (int)((100 * (loctm.tm_sec + ((double)tv.tv_usec / 1000000)) / 60)));

    // date
    strftime(buff, sizeof(buff), "%A %Y-%m-%02d", &loctm);
    putTextToCenter(54, buff);

    // set changed so we'll repaint on play
    setupPlayMode();
    refreshDisplay();
}

void cassettePage(A1Attributes *aio) {

    tagtypes_t a1layout[A1LINE_NUM][3] = {
        {TITLE, MAXTAG_TYPES, MAXTAG_TYPES},
        {COMPOSER, ARTIST, MAXTAG_TYPES},
    };

    char buff[BSIZE] = {0};
    char artist[255] = {0};
    char title[255] = {0};

    if (glopt->refreshLMS) {
        resetDisplay(1);
        softPlayRefresh(false);
        compactCassette();
    }

    setNagDone(false); // do not set refreshLMS
    softClockReset(false);
    softVisualizeRefresh(true);

    if (!aio->hubActive)
        aio->hubActive = true;

    // setup compound scrollers
    bool filled = false;
    bool changed = false;
    for (int line = 0; line < A1LINE_NUM; line++) {
        filled = false;
        int myline = line + 5;
        for (tagtypes_t *t = a1layout[line]; *t != MAXTAG_TYPES; t++) {
            if (tags[*t].valid) {
                filled = true;
                if (tags[*t].changed) {
                    changed = true;
                    if (0 == line)
                        strncpy(aio->title, tags[*t].tagData, 255);
                    else
                        strncpy(aio->artist, tags[*t].tagData, 255);
                }
            }
        }
    }

    if (changed) {
        sprintf(buff, "%s - %s", aio->artist, aio->title);
        if ((strcmp(buff, aio->compound) != 0) ||
            (0 == strlen(aio->compound))) // safe
        {
            putTextMaxWidth(19, 9, 15, aio->title);
            setSleepTime(SLEEP_TIME_SAVER);
        }
    }

    uint16_t pTime, dTime;
    int xpos = 54;
    int ypos = 24;
    int barmax = 15;
    drawRectangle(xpos, ypos, 21, 7, WHITE);
    drawRectangle(xpos, ypos, 21, 7, BLACK);
    pTime = (tags[TIME].valid) ? strtol(tags[TIME].tagData, NULL, 10) : 0;
    dTime =
        (tags[DURATION].valid) ? strtol(tags[DURATION].tagData, NULL, 10) : 0;
    // we want to emulate the tape moving from hub to hub as a track plays
    // wndow 19 wide, lHub starts at 15 wide, rHub starts at 0 wide
    // as the track progresses lHub shrinks, rHub expands
    // time and duration give us the percent complete -> transpose to emulate
    double pct = (pTime * 100.00) / (dTime == 0 ? 1 : dTime);
    // lHub
    drawHorizontalCheckerBar(xpos, ypos, barmax, 7, 100 - (int)pct);
    // rHub
    barmax = (int)(15 * (pct / 100.00));
    xpos +=
        (20 - barmax); // +right max and shift for 100% (right justified bar)
    drawHorizontalCheckerBar(xpos, ypos, barmax, 7, 100);

    audio_t audioDetail = {.samplerate = 44.1,
                           .samplesize = 16,
                           .audioIcon = 1}; // 2 HD 3 SD 4 DSD

    sampleDetails(&audioDetail);
    audioDetail.audioIcon = 1;
    if (1 == audioDetail.samplesize)
        audioDetail.audioIcon++;
    else if (16 != audioDetail.samplesize)
        audioDetail.audioIcon--;

    // type 1, 2 or 4 @ 100,29
    compactCassette();
    putTapeType(audioDetail);

    if ((pct > 99.6) && (aio->hubActive))
        aio->hubActive = false;
}

void allInOnePage(A1Attributes *aio) {

    /*
[I] 10%  [I][           ]
[HH:MM]     [           ]
[HH:MM]     [           ]
            [           ]
[Compound Track scroller]
*/

    tagtypes_t a1layout[A1LINE_NUM][3] = {
        {TITLE, MAXTAG_TYPES, MAXTAG_TYPES},
        {COMPOSER, ARTIST, MAXTAG_TYPES},
    };

    char buff[BSIZE] = {0};
    char artist[255] = {0};
    char title[255] = {0};

    DrawTime dt = {.charWidth = 12,
                   .charHeight = 17,
                   .bufferLen = LCD12X17_LEN,
                   .pos = {2, 10},
                   .font = MON_FONT_LCD1217};

    DrawTime rdt = {.charWidth = 12,
                    .charHeight = 17,
                    .bufferLen = LCD12X17_LEN,
                    .pos = {2, 28},
                    .font = MON_FONT_LCD1217};

    audio_t audioDetail = {.samplerate = 44.1,
                           .samplesize = 16,
                           .volume = -1,
                           .audioIcon = 3, // 2 HD 3 SD 4 DSD
                           .repeat = 0,
                           .shuffle = 0};

    sampleDetails(&audioDetail);
    softClockReset(false);

    // check softly -> paint a rectangle to cover visualization

    if (audioDetail.volume != glopt->lastVolume) {
        if (0 == audioDetail.volume)
            strncpy(buff, "  mute", 32);
        else
            sprintf(buff, "  %d%%  ", audioDetail.volume);
        putVolume((0 != audioDetail.volume), buff);
        setLastVolume(audioDetail.volume);
    }

    audioDetail.audioIcon = 3;
    if (1 == audioDetail.samplesize) {
        sprintf(buff, "DSD%.0f", (audioDetail.samplerate / 44.1));
        audioDetail.audioIcon++;
    } else {
        sprintf(buff, "%db/%.1f", audioDetail.samplesize,
                audioDetail.samplerate);
        if (16 != audioDetail.samplesize)
            audioDetail.audioIcon--;
    }

    strcpy(buff, " "); // this we'll ignore
    int8_t lastModes[2] = {audioDetail.shuffle, audioDetail.repeat};
    if ((strcmp(glopt->lastBits, buff) != 0) ||
        (glopt->lastModes[SHUFFLE_BUCKET] != lastModes[SHUFFLE_BUCKET]) ||
        (glopt->lastModes[REPEAT_BUCKET] != lastModes[REPEAT_BUCKET])) {
        putAudio(audioDetail, buff, false);
        setLastBits(buff);
        setLastModes(lastModes);
    }

    // setup compound scrollers
    bool filled = false;
    bool changed = false;
    for (int line = 0; line < A1LINE_NUM; line++) {
        filled = false;
        int myline = line + 5;
        for (tagtypes_t *t = a1layout[line]; *t != MAXTAG_TYPES; t++) {
            if (tags[*t].valid) {
                filled = true;
                if (tags[*t].changed) {
                    changed = true;
                    if (0 == line)
                        strncpy(aio->title, tags[*t].tagData, 255);
                    else
                        strncpy(aio->artist, tags[*t].tagData, 255);
                }
            }
        }
    }
    if (changed) {
        sprintf(buff, "%s - %s", aio->artist, aio->title);
        if ((strcmp(buff, aio->compound) != 0) ||
            (0 == strlen(aio->compound)) ||
            (!isScrollerActive(A1SCROLLER))) // safe
        {
            strncpy(aio->compound, buff, 255);
            setScrollPosition(A1SCROLLER, A1SCROLLPOS);
            if (putScrollable(A1SCROLLER, aio->compound)) {
                setSleepTime(SLEEP_TIME_SHORT);
            }
        }
    }

    // display time
    struct timeval tv;
    gettimeofday(&tv, NULL);
    time_t now = tv.tv_sec;
    struct tm loctm = *localtime(&now);

    // time
    sprintf(buff, "%02d:%02d", loctm.tm_hour, loctm.tm_min);
    if (strcmp(glopt->lastTime, buff) != 0)
        setLastTime(buff, dt);
    // colon (blink)
    drawTimeBlink(((loctm.tm_sec % 2) ? ' ' : ':'), &dt);

    uint16_t rTime =
        (tags[REMAINING].valid) ? strtol(tags[REMAINING].tagData, NULL, 10) : 0;

    // not hourly compliant!
    sprintf(buff, "%02d:%02d", rTime / 60, rTime % 60);
    setLastRemainingTime(buff, rdt);
    if (rTime < 2) // fingers crossed - won't catch a hard stop
        softClockReset(false);
}

#endif

void playingPage(void) {

    uint16_t pTime, dTime, rTime;
    char buff[BSIZE] = {0};

#ifdef __arm__

    audio_t audioDetail = {.samplerate = 44.1,
                           .samplesize = 16,
                           .volume = -1,
                           .audioIcon = 3, // 2 HD 3 SD 4 DSD
                           .repeat = 0,
                           .shuffle = 0};

    if (glopt->refreshLMS) {
        resetDisplay(1);
        softPlayRefresh(false);
    }

    sampleDetails(&audioDetail);

    if (glopt->downmix)
        setDownmix(audioDetail.samplesize, audioDetail.samplerate);
    else
        setDownmix(0, 0);

    if ((glopt->visualize) && (isVisualizeActive())) {
        if (activeScroller()) {
            scrollerPause();
        }
        if (glopt->refreshViz) {
            clearDisplay();
        }
        softVisualizeRefresh(false);
        softPlayReset();
        softClockReset(false);
        setNagDone();
        setSleepTime(SLEEP_TIME_SHORT);

    } else {
#endif
        sprintf(buff, "________________________\n");
        tOut(buff);

#ifdef __arm__

        setNagDone(false); // do not set refreshLMS
        softClockReset(false);
        softVisualizeRefresh(true);

        if (audioDetail.volume != glopt->lastVolume) {
            if (0 == audioDetail.volume)
                strncpy(buff, "  mute", 32);
            else
                sprintf(buff, "  %d%% ", audioDetail.volume);
            putVolume((0 != audioDetail.volume), buff);
            setLastVolume(audioDetail.volume);
        }

        audioDetail.audioIcon = 3;
        if (1 == audioDetail.samplesize) {
            sprintf(buff, "DSD%.0f", (audioDetail.samplerate / 44.1));
            audioDetail.audioIcon++;
        } else {
            sprintf(buff, "%db/%.1f", audioDetail.samplesize,
                    audioDetail.samplerate);
            if (16 != audioDetail.samplesize)
                audioDetail.audioIcon--;
        }

        if (strstr(buff, ".0") != NULL) {
            char *foo = NULL;
            foo = replaceStr(buff, ".0", ""); // cleanup a little
            sprintf(buff, "%s", foo);
            if (foo)
                free(foo);
        }

        int8_t lastModes[2] = {audioDetail.shuffle, audioDetail.repeat};
        if ((strcmp(glopt->lastBits, buff) != 0) ||
            (glopt->lastModes[SHUFFLE_BUCKET] != lastModes[SHUFFLE_BUCKET]) ||
            (glopt->lastModes[REPEAT_BUCKET] != lastModes[REPEAT_BUCKET])) {
            putAudio(audioDetail, buff);
            setLastBits(buff);
            setLastModes(lastModes);
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
                            setSleepTime(SLEEP_TIME_SHORT);
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
        setSleepTime(SLEEP_TIME_SHORT);
#endif

        pTime = (tags[TIME].valid) ? strtol(tags[TIME].tagData, NULL, 10) : 0;
        dTime = (tags[DURATION].valid)
                    ? strtol(tags[DURATION].tagData, NULL, 10)
                    : 0;
        rTime = (tags[REMAINING].valid)
                    ? strtol(tags[REMAINING].tagData, NULL, 10)
                    : 0;

#ifdef __arm__
        sprintf(buff, "%02d:%02d", pTime / 60, pTime % 60);
        int tlen = strlen(buff);
        clearLine(56);
        putText(1, 56, buff);

        // this is limited to sub hour programing - a stream may be 1+ hours
        if (glopt->remaining)
            sprintf(buff, "-%02d:%02d", rTime / 60, rTime % 60);
        else
            sprintf(buff, "%02d:%02d", dTime / 60, dTime % 60);

        int dlen = strlen(buff);
        putText(maxXPixel() - (dlen * charWidth()) - 1, 56, buff);

        sprintf(buff, "%s", (tags[MODE].valid) ? tags[MODE].tagData : "");
        int mlen = strlen(buff);
        putText(((maxXPixel() - ((tlen + mlen + dlen) * charWidth())) / 2) +
                    (tlen * charWidth()),
                56, buff);

        drawHorizontalBargraph(2, 51, maxXPixel() - 4, 4,
                               (pTime * 100) / (dTime == 0 ? 1 : dTime));
#endif

        sprintf(buff, "%3d:%02d  %5s  %3d:%02d", pTime / 60, pTime % 60,
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