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
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>

#include "common.h"
#include "visualize.h"
#include "visdata.h"
#include "display.h"

uint8_t cm = -1;
bool vis_is_active = false;
vis_type_t vis_mode = {0};
vis_type_t vis_list[3] =  {{0}, {0}, {0}};

size_t lenVisList(void) { return (sizeof(vis_list)/sizeof(vis_type_t)); }
void activateVisualizer(void) { vis_is_active = true; }
void setVisMode(vis_type_t mode) { strncpy(vis_mode, mode, 2); }
char getVisMode(void) { return *vis_mode; }
bool isVisualizeActive(void) { return vis_is_active; }
void deactivateVisualizer(void) { vis_is_active = false; }

void sayVisList(void)
{
    int ll = lenVisList();
    char delim[3] = {0};
    char stb[BSIZE] = {0};
    char say[BSIZE] = {0};

    for (int x=0; x < ll; x++)
    {
        if (!(isEmptyStr(vis_list[x])))
        {
            strcat(say, delim);
            if (strncmp(vis_list[x], MODE_RN, 2) == 0)
                strcat(say, "Random");
            else
                strcat(say, vis_list[x]);
            strcpy(delim, ", ");
        }
    }
    if (!(isEmptyStr(say)))
    {
        sprintf(stb, "Visualization ..: %s\n", say);
        putMSG(stb, LL_INFO);
    }
}

void setVisList(char *vlist)
{

    // init...
    int ll = lenVisList();
    for (int x=0; x < ll; x++)
    {
        vis_list[x][0] = '\0';
    }    
    
    int i = -1;
    char delim[3] = ",-";
    char *p = strtok (vlist, delim);
    while ((p != NULL) && (i+1 < ll))
    {
        strupr(p);
        // validate and silently assign to default if "bogus"
        if ((strncmp(p, MODE_VU, 2) != 0) 
        &&(strncmp(p, MODE_SA, 2) != 0)
        &&(strncmp(p, MODE_RN, 2) != 0)
        &&(strncmp(p, MODE_PK, 2) != 0)) {
            strncpy(p, MODE_VU, 2);
        }
        i++;
        strncpy(vis_list[i], p, 2);
        p = strtok (NULL, delim);
    }
    
}

char currentMeter(void)
{
    cm++;
    if (cm > lenVisList()-1)
        cm = 0;

    if (isEmptyStr(vis_list[cm]))
        cm = 0;

    if (strcmp(vis_list[cm],MODE_RN) == 0)
    {
        // pick a random visualization
        int i = rand()%3;
        switch (i)
        {
            case 0: setVisMode(MODE_VU); break;
            case 1: setVisMode(MODE_SA); break;
            case 2: setVisMode(MODE_PK); break;
            default:
                setVisMode(MODE_VU);
        }
    }
    else
        setVisMode(vis_list[cm]);

    return getVisMode();

}

int visgood = 0;
void visualize(struct vissy_meter_t *vissy_meter) {

/*
char stb[BSIZE];
sprintf(stb,"(active)%02d (pop)%02d :: (data) %2s -> (active) %2s\n",
vis_is_active, visgood, vissy_meter->meter_type, vis_mode); 
putMSG(stb,LL_INFO);
*/
    if ((vis_is_active) && (visgood < 3)) {

        // bug workaround
        if (isEmptyStr(vis_mode))
            currentMeter();

        if (strncmp(MODE_VU, vissy_meter->meter_type, 2) == 0) {
            // support vu or pk
            if (strncmp(vis_mode, MODE_VU, 2) == 0)
                stereoVU(vissy_meter);
            else
                if (strncmp(vis_mode, MODE_PK, 2) == 0)
                    stereoPeakH(vissy_meter);
        }
        else 
            if (strncmp(vis_mode,vissy_meter->meter_type,2) == 0)
                stereoSpectrum(vissy_meter);
        
    }

    // stream is too fast for display - a 1:20 consumption ratio plays happy w/ display
    // routine VU routine checks for change so we may be Ok, parameterize and push limits
    // is worse still on pi3 build...
    visgood++;
    if (visgood > 19) // sweet spot for SA
        visgood = 0;

}
