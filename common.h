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

#ifndef COMMON_H
#define COMMON_H 1

#define BSIZE 4096

#define LL_QUIET 0
#define LL_INFO 1
#define LL_DEBUG 2

int incVerbose(void);
int getVerbose(void);
int putMSG(const char *msg, int loglevel);
void enableTOut(void);
int tOut(const char *msg);
void abort(const char *msg);
char *replaceStr(const char *s, const char *find, const char *replace);

#endif
