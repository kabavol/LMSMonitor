/*
 *	(c) 2020 Stuart Hunter
 *
 *  Capture ISP IP, we assume its close (enough)
 *  Use the IP to derive lat/long
 *  Use lat/lng and local time to calculate sunrise and sunset
 *  Set display brightness based on time of day and sunlight
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

#ifndef ASTRAL_H
#define ASTRAL_H

#include <stdbool.h>
#include <stdint.h>

#include <sys/time.h>
#include <time.h>

#include "climacell.h"

typedef struct isp_locale_t {
    time_t sunrise;
    time_t sunset;
    int daybright;
    int nightbright;
    coord_t coords;
    int brightness;
    char Timezone[128]; // string not GMT offset!
} isp_locale_t;

typedef enum Method { UNSUPPORTED, GET, HEAD, POST } Method;

typedef struct Header {
    char *name;
    char *value;
    struct Header *next;
} Header;

typedef struct Request {
    enum Method method;
    char *url;
    char *version;
    struct Header *headers;
    char *body;
} Request;

struct Request *parse_request(const char *raw);
void free_header(struct Header *h);
void free_request(struct Request *req);

bool initAstral(void);
void brightnessEvent(void);

void weatherEvent(struct climacell_t *climacell);
bool updClimacell(climacell_t *climacell);

#endif