/*
 *	log.c
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

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "vissy.h"
#include "log.h"

time_t log_oldrawtime = 1;
char log_timebuf[80];

struct vissy_settings *plogvisset;

void logInit(struct vissy_settings *se) { plogvisset = se; }

void toLog(int level, const char *format, ...) {
  if (level <= plogvisset->loglevel) {
    if (plogvisset->daemon) {
      FILE *logf;

      logf = fopen(plogvisset->logfile, "a");
      if (logf == NULL) {
        fprintf(stderr, "Error opening log file: %s\n", plogvisset->logfile);
        exit(1);
      }

      time_t rawtime;
      time(&rawtime);
      if (log_oldrawtime != rawtime) {
        struct tm *timeinfo;
        timeinfo = localtime(&rawtime);
        strftime(log_timebuf, 80, "%F %T visionon: ", timeinfo);
        log_oldrawtime = rawtime;
      }

      va_list args;
      va_start(args, format);
      fputs(log_timebuf, logf);
      vfprintf(logf, format, args);
      va_end(args);
      fclose(logf);
    } else {
      va_list args;
      va_start(args, format);
      vfprintf(stdout, format, args);
      va_end(args);
      fflush(stdout);
    }
  }
}
