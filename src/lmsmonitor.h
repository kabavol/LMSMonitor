/*
 *	lmsmonitor.h
 *
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

#ifndef LMSMON_H
#define LMSMON_H

#include <stdint.h>

// egg modes (animated playback emulations)
#define EE_NONE 0
#define EE_CASSETTE 1
#define EE_VINYL 2
#define EE_REEL2REEL 3
#define EE_MAX 4

bool isPlaying(void);
bool lockOptions(void);
bool acquireOptLock(void);

typedef struct A1Attributes {
    uint8_t eeMode;
    bool eeFXActive;
    int8_t lFrame;
    int8_t rFrame;
    int8_t mxframe;
    int8_t flows;
    char artist[255];
    char title[255];
    char compound[255];
} A1Attributes;

// should not be called externally... or should it...
void allInOnePage(A1Attributes *aio);

#endif