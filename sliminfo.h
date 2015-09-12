/*
 *	(c) 2015 László TÓTH
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

#ifndef SLIMINFO_H
#define SLIMINFO_H

#define MAXTAG_DN	16
#define MAXTAG_DATA	255

typedef struct Tag {
	const char *name;
	const char *displayName;
	char *tagData;
	int  valid;
	int  changed;
} tag;

typedef enum {SAMPLESIZE, SAMPLERATE, TIME, DURATION, TITLE, ALBUM, ARTIST, ALBUMARTIST, COMPOSER, CONDUCTOR, MODE, LMSVOLUME, MAXTAG_TYPES} tagtypes_t;

void  closeSliminfo(void);
tag  *initSliminfo(char *playerName);
void  error(const char *msg);
void  askRefresh(void);
int   isRefreshed(void);

#endif
