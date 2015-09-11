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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <limits.h>

#include "common.h"

int verbose = 0;
int textOut = false;

int incVerbose(void) {
	if (verbose < INT_MAX) {
		verbose++;
	}
	return verbose;
}

int getVerbose(void) {
	return verbose;
}

int putMSG (const char *msg, int loglevel) {
	if (loglevel > verbose) {return false;}
	printf ("%s", msg);
	return true;
}

void enableTOut(void) {
	textOut = true;
}

int tOut(const char *msg) {
	if (!textOut) 	{return false;}
	printf ("%s", msg);
	return true;
}

void abort(const char *msg) {
	perror(msg);
	exit(1);
}
