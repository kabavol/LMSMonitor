/*
 *	(c) 2020 Stuart Hunter
 *
 *  Weather data provided by climacell
 *  api key and units passed as CLI attributes
 *  lat/lng is derived via an ISP lookup - same as the astral data
 *  those routines are colocated given JSON primitives are utilized
 *  by both interfaces.
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

#ifndef CLIMACELL_H
#define CLIMACELL_H

#include <stdint.h>
/////#include "astral.h"


typedef struct coord_t {
    double Latitude;
    double Longitude;
} coord_t;

typedef struct ccdatum_t {
    char sdatum[128];
    double fdatum;
    char units[128];
} ccdatum_t;

// climacell free tier :: 1000 calls per day
// lookup at 10-15 minute intervals more than sufficient

/*
{"lat":42.361422,"lon":-71.10409,"temp":{"value":66.43,"units":"F"},
"feels_like":{"value":66.43,"units":"F"},"wind_speed":{"value":4.47,"units":"mph"},
"baro_pressure":{"value":30.0911,"units":"inHg"},"visibility":{"value":6.21371,"units":"mi"},
"humidity":{"value":83.31,"units":"%"},"wind_direction":{"value":57.94,"units":"degrees"},
"precipitation":{"value":0,"units":"in/hr"},"sunrise":{"value":"2020-08-16T09:53:20.530Z"},
"sunset":{"value":"2020-08-16T23:43:29.297Z"},"weather_code":{"value":"mostly_clear"},
"observation_time":{"value":"2020-08-16T02:28:59.515Z"}}
*/

typedef struct climacell_t {
    char Apikey[64];
    char host[128];
    char uri[128];
    char units[3];
    coord_t coords;
	char fields[1024];
    uint8_t icon;
    ccdatum_t temp;
    ccdatum_t feels_like;
    ccdatum_t wind_speed;
    ccdatum_t baro_pressure;
    ccdatum_t humidity;
    ccdatum_t wind_direction;
    ccdatum_t precipitation;
    ccdatum_t weather_code;
} climacell_t;

/*
weather_xlate = {
"rain_heavy":"Heavy Rain",
"rain":"Rain",
"rain_light":"Light Rain",
"freezing_rain_heavy":"Freezing Rain/Sleet",
"freezing_rain":"Freezing Rain",
"freezing_rain_light":"Light Freezing Rain",
"freezing_drizzle":"Freezing Drizzle",
"drizzle":"Drizzle",
"ice_pellets_heavy":"Hail".
"ice_pellets":"Hail",
"ice_pellets_light":"Light Hail",
"snow_heavy":"Heavy Snow",
"snow":"Snow",
"snow_light":"Snow Showers",
"flurries":"Snow Flurries",
"tstorm":"Thunderstorm",
"fog_light":"Light Fog",
"fog":"Fog",
"cloudy":"Cloudy",
"mostly_cloudy":"Mostly Cloudy",
"partly_cloudy":"Partly Cloudy", # partly_cloudy, partly_cloudy_night
"mostly_clear":"Mostly Clear",	# mostly_clear_day, mostly_clear_night
"clear":"Clear, Sunny",	# clear_day, clear_night
}
*/

#endif