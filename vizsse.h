
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

#ifndef VISSY_VIZSSE_H
#define VISSY_VIZSSE_H

#define MAX_HEADERS 100
#define RESPONSE_LIMIT  128 * 1024

#include <stdarg.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <stdio.h>

#include "vissy.h"
#include "visdata.h"

#define VERSION "0.4.8"

struct Options {
  const char *pname;          // process name
  const char *uri;            // URI to get
  int         limit;          // event limit
  int         verbosity;      // verbosity
  int         allow_insecure; // allow insecure connections - not for this impl.
  const char *ssl_cert;       // SSL cert file - not for this impl.
  const char *ca_info;        // CA cert file - not for this impl.
};

struct MemoryStruct {
  char *memory;
  size_t size;
};

#define Options_Initializer {0,0,0,0,0,0,0}
DECLARE_OBJECT(Options, options);

#define FD_STDIN    0
#define FD_STDOUT   1
#define FD_STDERR   2

static void parseSSEArguments(int argc, char** argv);
bool setupSSE(int argc, char** argv);
void vissySSEFinalize(void);

// SSE data processing callbacks
extern void parse_payload(char *ptr, size_t size);
extern void on_sse_event(char** headers, const char* data);

// data -> parse -> visualization 
void parse_visualize(char *data, struct vissy_meter_t *vissy_meter);

#endif

