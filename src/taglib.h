/*
 *	(c) 2020 Stuart Hunter
 *
 *	Todo:
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

#ifndef TAGLIB_H
#define TAGLIB_H

#define MAXTAG_DATA 255

#include <stdbool.h>
#include <stdint.h>
#include "common.h"

typedef struct player_t {
    char playerName[BSIZE];
    char playerIP[BSIZE];
    char playerID[BSIZE];
    bool online;
} player_t;

typedef struct lms_t {
    int playerCount;
    int activePlayer;
    player_t players[10]; // max 9 players - hopefully more than sufficient
    bool refresh;
    bool serverOnline;
    uint16_t LMSPort;
    char LMSHost[128];
    char body[BSIZE];
} lms_t;

typedef struct tag_t {
    const char *name;
    int8_t keyLen;
    char *tagData;
    bool valid;
    bool changed;
} tag_t;

typedef enum tagtypes_t {
    ARTIST,
    ALBUMARTIST,
    COMPOSER,
    CONDUCTOR,
    ALBUM,
    TITLE,
    TIME,
    DURATION,
    REMAINING,
    VOLUME,
    COMPILATION,
    YEAR,
    SAMPLESIZE,
    SAMPLERATE,
    MODE,
    PERFORMER,
    REPEAT,
    SHUFFLE,
    CONNECTED,
    TRACKNUM,
    TRACKARTIST,
    REMOTE,
    TRACKID, // unique ID!!!
    MAXTAG_TYPES
} tagtypes_t;

#endif
