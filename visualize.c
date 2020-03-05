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

#include "common.h"
#include "visualize.h"
#include <sys/time.h>
#include <time.h>

#include "display.h"

bool vis_is_active = false;

void activateVisualizer(void)
{
    vis_is_active = true;
}

bool isVisualizeActive(void) { return vis_is_active; }

void deactivateVisualizer(void)
{
    vis_is_active = false;
}

int visgood = 0;
void visualize(struct vissy_meter_t *vissy_meter) {

    if((vis_is_active)&&(1 == visgood)){
        stereoVU(vissy_meter);
    }

    // stream too fast for display - 1:40 ration plays happy with display
    visgood++;
    if (visgood > 39)
        visgood = 0;

}
