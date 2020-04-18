/*
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

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "common.h"
#include "display.h"
#include "visdata.h"
#include "visualize.h"
////#include "hash.h"

uint8_t cm = -1;
bool vis_is_active = false;
vis_type_t vis_mode = {0};
vis_type_t vis_list[5] = {{0}, {0}, {0}, {0}, {0}};
uint8_t num_modes = 1;
char downmix[5] = {0};

size_t lenVisList(void) { return (sizeof(vis_list) / sizeof(vis_type_t)); }
void activateVisualizer(void) {
    instrument(__LINE__, __FILE__, "Visualize On ->");
    setInitRefresh();
    vis_is_active = true;
    instrument(__LINE__, __FILE__, "Visualize On <-");
}

bool setVisMode(vis_type_t mode) {
    bool ret = false;
    if (strcmp(vis_mode, mode) != 0) {
        strncpy(vis_mode, mode, 2);
        ret = true;
    }
    return ret;
}

const char *getVisMode(void) {
    if (!isEmptyStr(vis_mode))
        return vis_mode;
    else
        return "??";
}

const bool isVisualizeActive(void) { return vis_is_active; }

void deactivateVisualizer(void) {
    instrument(__LINE__, __FILE__, "->Visualize Off");
    vis_is_active = false;
}

void setDownmix(int samplesize, float samplerate) {
    if (0 == samplesize)
        strcpy(downmix, "N");
    else if (1 == samplesize)
        sprintf(downmix, "D%.0f", (samplerate / 44.1));
    else
        sprintf(downmix, "M%d", samplesize);
}

void sayVisList(void) {
    int ll = lenVisList();
    char delim[3] = {0};
    char stb[BSIZE] = {0};
    char say[BSIZE] = {0};

    for (int x = 0; x < ll; x++) {
        if (!(isEmptyStr(vis_list[x]))) {
            strcat(say, delim);
            if (strncmp(vis_list[x], VMODE_RN, 2) == 0)
                strcat(say, "Random");
            else
                strcat(say, vis_list[x]);
            strcpy(delim, ", ");
        }
    }
    if (!(isEmptyStr(say))) {
        sprintf(stb, "%s %s\n", labelIt("Visualization", LABEL_WIDTH, "."),
                say);
        putMSG(stb, LL_INFO);
    }
}

void setVisList(char *vlist) {

    // init...

    int ll = lenVisList();
    for (int x = 0; x < ll; x++) {
        vis_list[x][0] = 0; //'\0';
    }

    int i = -1;
    char delim[3] = ",-";
    char *p = strtok(vlist, delim);
    while ((p != NULL) && (i + 1 < ll)) {
        strupr(p);
        // validate and silently assign to default if "bogus"
        if ((strncmp(p, VMODE_VU, 2) != 0) && (strncmp(p, VMODE_SA, 2) != 0) &&
            (strncmp(p, VMODE_ST, 2) != 0) && (strncmp(p, VMODE_RN, 2) != 0) &&
            (strncmp(p, VMODE_PK, 2) != 0) && (strncmp(p, VMODE_SM, 2) != 0)) {
            strcpy(p, VMODE_NA);
        }
        i++;
        strncpy(vis_list[i], p, 2);
        p = strtok(NULL, delim);
    }
    num_modes = i;
}

char *currentMeter(void) {
    cm++;
    if (cm > lenVisList() - 1)
        cm = 0;

    if (isEmptyStr(vis_list[cm]))
        cm = 0;

    if (strcmp(vis_list[cm], VMODE_RN) == 0) {
        // pick a random visualization
        switch ((rand() % VEMODE_MX)+1) {
            case VEMODE_VU: setVisMode(VMODE_VU); break;
            case VEMODE_SA: setVisMode(VMODE_SA); break;
            case VEMODE_ST: setVisMode(VMODE_ST); break;
            case VEMODE_SM: setVisMode(VMODE_SM); break;
            default: setVisMode(VMODE_PK);
        }
    } else
        setVisMode(vis_list[cm]);

    if (debugActive()) {
        char stb[128] = {0};
        sprintf(stb, "%s %s\n", labelIt("Active Visualizer", LABEL_WIDTH, "."),
                getVisMode());
        putMSG(stb, LL_DEBUG);
    }

    return (char *)getVisMode();
}

int visgood = 0;
bool lastTest = false;
void visualize(struct vissy_meter_t *vissy_meter) {

    if (vis_is_active) {

//        if (0 == (visgood % 3)) {

            if (isEmptyStr(downmix)) {
                strcpy(downmix, "N");
                instrument(__LINE__, __FILE__, "<-Fixed Downmix");
            }
            if (isEmptyStr(vis_mode)) {
                currentMeter();
                instrument(__LINE__, __FILE__, "<-Fixed VisMode");
            }

            switch(getMode(vissy_meter->meter_type))
            {
            case VEMODE_VU:
                switch(getMode(vis_mode))
                {
                case VEMODE_VU:
                    instrument(__LINE__, __FILE__, "->Visualize VU");
                    stereoVU(vissy_meter, downmix);
                    instrument(__LINE__, __FILE__, "<-Visualize VU");
                    break;
                case VEMODE_PK:
                    instrument(__LINE__, __FILE__, "->Visualize PK");
                    stereoPeakH(vissy_meter, downmix);
                    instrument(__LINE__, __FILE__, "<-Visualize PK");
                    break;
                }
                break;
            case VEMODE_SA:
                switch(getMode(vis_mode))
                {
                case VEMODE_SA:
                    instrument(__LINE__, __FILE__, "->Visualize SA");
                    stereoSpectrum(vissy_meter, downmix);
                    instrument(__LINE__, __FILE__, "<-Visualize SA");
                    break;
                case VEMODE_ST:
                    instrument(__LINE__, __FILE__, "->Visualize ST");
                    ovoidSpectrum(vissy_meter, downmix);
                    instrument(__LINE__, __FILE__, "<-Visualize ST");
                    break;
                case VEMODE_SM:
                    instrument(__LINE__, __FILE__, "->Visualize SM");
                    mirrorSpectrum(vissy_meter, downmix);
                    instrument(__LINE__, __FILE__, "<-Visualize SM");
                    break;
                }
            }
        }
        lastTest = true;
    
    //}
    // stream is too fast for display
    // a 1:20 consumption ratio plays
    // happier w/ the display
    //visgood++;
    //if (visgood > 20)
    //    visgood = 0;
}
