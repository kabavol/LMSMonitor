
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

#ifndef VISSY_VIZSSET_H
#define VISSY_VIZSSET_H

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "../src/visdata.h"
#include "../src/vissy.h"

#define MAX_HEADERS 100
#define RESPONSE_LIMIT 128 * 1024

struct Options {
    const char *uri; // URI to get
    const char *host;
    int port;
    const char *endpoint;
    int limit;          // event limit
    int verbosity;      // verbosity
    int allow_insecure; // allow insecure connections - not for this impl.
    //const char *ssl_cert;       // SSL cert file - not for this impl.
    //const char *ca_info;        // CA cert file - not for this impl.
};

bool setupSSE(struct Options opt);
void vissySSEFinalize(void);

extern void parse_payload(char *data, uint64_t len);
//extern void on_sse_event(char** headers, const char* data);
extern void on_sse_event(const char *data);

// data -> parse -> visualization
void parse_visualize(char *data, struct vissy_meter_t *vissy_meter);

#endif
