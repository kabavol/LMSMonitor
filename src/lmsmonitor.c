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
struct MonitorAttrs *glopt;

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
        " -b automatically set brightness of display at sunset and sunrise (pi "
        "only)\n"
        " -c display clock when not playing (Pi only)\n"
        " -d downmix audio and display a single large meter, SA and VU only\n"
        " -i increment verbose level\n"
        " -k show CPU load and temperature (clock mode)\n"
        " -m if visualization on specify one or more meter modes, sa, vu, "
        "pk, st, or rn for random\n"
        " -o specifies OLED \"driver\" type (see options below)\n"
        " -r show remaining time rather than track time\n"
        " -S scrollermode: 0 (cylon), 1 (infinity left), 2 infinity (right)\n"
        " -v enable visualization sequence when playing (Pi only)\n"
        " -x specifies OLED address if default does not work - use i2cdetect to "
        "find address (Pi only)\n"
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

bool acquireOptLock(void) 
{

    char buff[128] = {0};
    bool ret = true;
    if (glopt)
    {
        uint8_t test = 0;
        while (pthread_mutex_trylock(&glopt->update) != 0)
        {
            if (test>30)
            {
                ret = false;
                strcpy(buff,"options mutex acquire failed\n");
                putMSG(buff, LL_DEBUG);
                break;
            }
            usleep(10);
            test++;
        }
    }
    return ret;

}

void softClockRefresh(bool rc=true)
{
    if (rc != glopt->refreshClock)
        if (acquireOptLock()) {
            glopt->refreshClock = rc;
            pthread_mutex_unlock(&glopt->update);
        }
}

void softVisualizeRefresh(bool rv=true)
{
    if (glopt->visualize)
        if (rv != glopt->refreshViz)
            if (acquireOptLock()) {
                glopt->refreshViz = rv;
                pthread_mutex_unlock(&glopt->update);
            }
}

void softPlayRefresh(bool r=true)
{
    if (r != glopt->refreshLMS)
        if (acquireOptLock()) {
            glopt->refreshLMS = r;
            pthread_mutex_unlock(&glopt->update);
        }
}

void setLastTime(char *tm)
{
    if (strcmp(glopt->lastTime, tm) != 0)
    {
        if (acquireOptLock()) {
            drawTimeTextL(tm, glopt->lastTime, ((glopt->showTemp) ? -1 : 1));
            //drawTimeTextS(tm, glopt->lastTime, 17);
            strncpy(glopt->lastTime, tm, 32);
            pthread_mutex_unlock(&glopt->update);
        }
    }
}

void setSleepTime(int st)
{
    if(st != glopt->sleepTime)
        if (acquireOptLock()) {
            glopt->sleepTime = st;
            pthread_mutex_unlock(&glopt->update);
        }
}

void putCPUMetrics(int y)
{
    if (acquireOptLock()) {
        sprintf(glopt->lastLoad, "%.0f%%", cpuLoad());
        sprintf(glopt->lastTemp, "%.1fC", cpuTemp());
        char buff[BSIZE] = {0};
        sprintf(buff, "%s %s", glopt->lastLoad, glopt->lastTemp);
        putTextCenterColor(y, buff, WHITE);
        pthread_mutex_unlock(&glopt->update);
    }
}

void softClockReset(bool cd=true)
{
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

void setLastVolume(int vol)
{
    if (vol != glopt->lastVolume)
        if (acquireOptLock()) {
            glopt->lastVolume = vol;
            pthread_mutex_unlock(&glopt->update);
        }
}

void setLastBits(char *lb)
{
    if (strcmp(lb, glopt->lastBits) !=0)
        if (acquireOptLock()) {
            strncpy(glopt->lastBits, lb, 32);
            pthread_mutex_unlock(&glopt->update);
        }
}

void softPlayReset(void)
{
    if (acquireOptLock()) {
        glopt->lastBits[0] = 0;
        glopt->lastVolume = -1;
        glopt->refreshLMS = true;
        pthread_mutex_unlock(&glopt->update);
    }
}

void setNagDone(bool refresh=true)
{
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
            softlySoftly = true;
            softVisualizeRefresh(true);
        }
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
                instrument(__LINE__, __FILE__,
                           "Active Visualization");
                vis_type_t current = {0};
                vis_type_t updated = {0};
                strncpy(current, getVisMode(), 2);
                strncpy(updated, currentMeter(), 2);
                if (strcmp(current, updated) != 0) {
                    softlySoftly = true;
                    ///softClear();
                }
            } else {
                currentMeter();
            }
            softVisualizeRefresh(true);
        } else {
            currentMeter();
        }
    }
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
        .refreshViz = false,
        .lastVolume = -1,
    };

    if (pthread_mutex_init(&lmsopt.update, NULL) != 0) {
        closeDisplay();
        printf("\nLMS Options mutex init has failed\n");
        exit(EXIT_FAILURE);
    }

    strcpy(lmsopt.lastBits, "LMS");
    strcpy(lmsopt.lastTemp, "00000");
    strcpy(lmsopt.lastTime, "XX:XX");

    glopt = &lmsopt;

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
    while ((aName = getopt(argc, argv, "n:o:m:x:B:S:abcdhikprtvz")) != -1) {
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
                printf("All-In-One coming soon\n"); 
                lmsopt.allInOne = true; 
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

            case 'k': lmsopt.showTemp = true; break;

            case 'r': lmsopt.remaining = true; break;

            case 'x':
                lmsopt.oledAddrL = (int)strtol(optarg, NULL, 16);
                //lmsopt.oledAddrR = (int)strtol(hexstr1, NULL, 16);
                setOledAddress(lmsopt.oledAddrL);
                break;
            case 'z': lmsopt.splash = false; break;

            case 'S':
                short sm; 
                sscanf(optarg, "%d", &sm);
                setScrollMode(sm); 
                break;
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
    sprintf(stbl, "%s %s\n", 
        labelIt("Verbosity", LABEL_WIDTH, "."),
        getVerboseStr());
    putMSG(stbl, LL_QUIET);
    printOledSetup(); // feedback and "debug"
    printScrollerMode(); // feedback and "debug"
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

    // if we had a clean setup
    // init visualizer mode and
    // timer toggle

    if (lmsopt.visualize) {
        sayVisList();
        size_t viztimer;
        size_t vizcycletimer;
        timer_initialize();
        viztimer = timer_start(60 * 1000, toggleVisualize, TIMER_PERIODIC,
                               (void *)NULL);
        vizcycletimer = timer_start(99 * 1000, cycleVisualize, TIMER_PERIODIC, 
                               (void *)NULL);

        sprintf(stbl, "%s %s\n", labelIt("Downmix VU+SA", LABEL_WIDTH, "."),
                ((lmsopt.downmix) ? "Yes" : "No"));

    } else {
        sprintf(stbl, "%s Inactive\n",
                labelIt("Visualization", LABEL_WIDTH, "."));
    }
    putMSG(stbl, LL_INFO);

    clearDisplay(); // clears the  splash if shown

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
            playing = (strcmp("play", tags[MODE].tagData) == 0);

            if (playing) {

#ifdef __arm__

                instrument(__LINE__, __FILE__, "isPlaying");

                // Threaded logic in play - DO NOT MODIFY
                if (!isVisualizeActive())
                    playingPage();
                else
                    setupPlayMode();
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
                instrument(__LINE__, __FILE__, "clock test");
                if (lmsopt.clock)
                    clockPage();
                else
                    saverPage();
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
        setLastTime(buff);

    // colon (blink)
    drawTimeBlinkL(((loctm.tm_sec % 2) ? ' ' : ':'),((glopt->showTemp) ? -1 : 1));
    //drawTimeBlinkS(((loctm.tm_sec % 2) ? ' ' : ':'), 17);

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
#endif

void playingPage(void)
{

    long actVolume = 0;
    int audio = 3; // 2 HD 3 SD 4 DSD
    long pTime, dTime, rTime;
    char buff[255];

#ifdef __arm__

    if (glopt->refreshLMS) {
        resetDisplay(1);
        softPlayRefresh(false);
    }

    // output sample rate and bit depth too, supports DSD nomenclature
    double samplerate = tags[SAMPLERATE].valid
                            ? strtof(tags[SAMPLERATE].tagData, NULL) / 1000
                            : 44.1;
    int samplesize = tags[SAMPLESIZE].valid
                         ? strtol(tags[SAMPLESIZE].tagData, NULL, 10)
                         : 16;

    if (glopt->downmix)
        setDownmix(samplesize, samplerate);
    else
        setDownmix(0, 0);

    if ((glopt->visualize) && (isVisualizeActive())) {
        if (activeScroller()) {
            scrollerPause();
        }
        if (glopt->refreshViz)
        {
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

        actVolume =
            tags[VOLUME].valid ? strtol(tags[VOLUME].tagData, NULL, 10) : 0;
        if (actVolume != glopt->lastVolume) {
            if (0 == actVolume)
                strncpy(buff, "  mute", 32);
            else
                sprintf(buff, "  %ld%% ", actVolume);
            putVolume((0 != actVolume), buff);
            setLastVolume(actVolume);
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

        if (strcmp(glopt->lastBits, buff) != 0) {
            putAudio(audio, buff);
            setLastBits(buff);
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
        sprintf(buff, "%02ld:%02ld", pTime / 60, pTime % 60);
        int tlen = strlen(buff);
        clearLine(56);
        putText(1, 56, buff);

        // this is limited to sub hour programing - a stream may be 1+ hours
        if (glopt->remaining)
            sprintf(buff, "-%02ld:%02ld", rTime / 60, rTime % 60);
        else
            sprintf(buff, "%02ld:%02ld", dTime / 60, dTime % 60);

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
