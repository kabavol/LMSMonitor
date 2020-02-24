/*
 *	(c) 2015 László TÓTH
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

#ifndef TAGUTILS_H
#define TAGUTILS_H

#include "sliminfo.h"

char *getTag(const char *tag, char *input, char *output, int outSize);
char *getQuality(char *input, char *output, int outSize);
int isPlaying(char *input);
long getMinute(tag *timeTag);
void encode(const char *s, char *enc);
int decode(const char *s, char *dec);

#endif
