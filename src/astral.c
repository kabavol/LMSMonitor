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

#include <arpa/inet.h>
#include <assert.h>
#include <math.h>
#include <netdb.h> // getprotobyname
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// compile with -lssl -lcrypto for https support

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/ssl.h>

#include "astral.h"
#include "basehttp.h"
#include "climacell.h"
#include "common.h"
#include "display.h"

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

#ifndef JSMN_STATIC
#define JSMN_STATIC
#endif

#include "jsmn.h"

static int jsonEq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
}

bool daymode = true;
isp_locale_t isp_locale;

void baseCCDatum(ccdatum_t *datum, bool changed, const char *key,
                 const char *lbl) {
    datum->changed = changed;
    if (!datum->key) {
        datum->key = key;
        datum->lenk = strlen(key);
        datum->fdatum = -99.876543;
        strcpy(datum->sdatum, "xxx");
        strcpy(datum->units, "xxx");
    }
    if (!datum->lbl)
        datum->lbl = lbl;
}

bool copyCCDatum(ccdatum_t *datum, ccdatum_t *from) {
    if ((from->changed) || (datum->fdatum != from->fdatum)) {
        datum->fdatum = from->fdatum;
        strcpy(datum->sdatum, from->sdatum);
        strcpy(datum->units, from->units);
        datum->changed = from->changed;
        return true;
    }
    return false;
}

void baselineClimacell(climacell_t *climacell, bool changed) {

    climacell->current.changed = changed;

    baseCCDatum(&climacell->temp, changed, "temp", "weather:Temperature");
    baseCCDatum(&climacell->feels_like, changed, "feels_like",
                "weather:Feels Like");
    baseCCDatum(&climacell->wind_speed, changed, "wind_speed",
                "weather:Wind Speed");
    baseCCDatum(&climacell->baro_pressure, changed, "baro_pressure",
                "weather:Pressure");
    baseCCDatum(&climacell->humidity, changed, "humidity",
                "weather:Humidity");
    baseCCDatum(&climacell->visibility, changed, "visibility",
                "weather:Visibility");
    baseCCDatum(&climacell->wind_direction, changed, "wind_direction",
                "weather:Wind Dir");
    baseCCDatum(&climacell->precipitation, changed, "precipitation",
                "weather:Precip");
    baseCCDatum(&climacell->weather_code, changed, "weather_code",
                "weather:Code");
    // don't need change functionality but key/label are useful
    baseCCDatum(&climacell->observation_time, changed, "observation_time",
                "weather:Time");
}

void decodeKV(char *jsonData, ccdatum_t *datum, int i, int j, jsmntok_t jt[]) {

    // decode value pair + units group

    ccdatum_t testdatum;
    testdatum.fdatum = datum->fdatum;
    strcpy(testdatum.sdatum, datum->sdatum);
    strcpy(testdatum.units, datum->units);
    datum->changed = false;

    for (int z = i; z < j; z++) {

        // key
        jsmntok_t tok = jt[z];
        uint16_t lenk = tok.end - tok.start;
        char keyStr[lenk + 1];
        memcpy(keyStr, &jsonData[tok.start], lenk);
        keyStr[lenk] = '\0';
        // (value)
        tok = jt[z + 1];
        uint16_t lenv = tok.end - tok.start;
        char valStr[lenv + 1];
        memcpy(valStr, &jsonData[tok.start], lenv);
        valStr[lenv] = '\0';
        // nibble so fits detail view
        if (strncmp(valStr, "in/hr", 5) == 0)
            strcpy(valStr, "in");
        else if (strncmp(valStr, "cm/hr", 5) == 0)
            strcpy(valStr, "cm");
        else if (strncmp(valStr, "mm/hr", 5) == 0)
            strcpy(valStr, "mm");

        if (strncmp("value", keyStr, 5) == 0) {
            if (strcmp(testdatum.sdatum, valStr) != 0) {
                strcpy(datum->sdatum, valStr);
                datum->fdatum = atof(valStr);
                datum->changed = true;
            }
            z++;
        } else if (strncmp("units", keyStr, 5) == 0) {
            if (strcmp(testdatum.units, valStr) != 0) {
                strcpy(datum->units, valStr);
                datum->changed = true;
            }
            z++;
        }
    }
}

static const char *degToCompass(double deg) {
    int d16 = (int)((deg / 22.5) + .5);
    d16 %= 16;
    static const char windd[17][4] = {"N",  "NNE", "NE", "ENE", "E",  "ESE",
                                      "SE", "SSE", "S",  "SSW", "SW", "WSW",
                                      "W",  "WNW", "NW", "NNW"};
    return (const char *)windd[d16];
}

wiconmap_t weatherIconXlate(char *key) {

    // add day night logic for specifics
    char wspecific[128] = {0};
    sprintf(wspecific, "%s%s", key, (daymode) ? "_day" : "_night");

    const static struct wiconmap_t wmap[] = {
        (wiconmap_t){"clear_day", "Clear", 0, true},
        (wiconmap_t){"clear_night", "Clear", 1, true},
        (wiconmap_t){"clear", "Clear", 0, true},
        (wiconmap_t){"cloudy", "Cloudy", 2, true},
        (wiconmap_t){"drizzle", "Drizzle", 3, true},
        (wiconmap_t){"flurries", "Snow Flurries", 4, true},
        (wiconmap_t){"fog", "Fog", 5, true},
        (wiconmap_t){"fog_light", "Mist", 6, true},
        (wiconmap_t){"freezing_drizzle", "Freezing Drizzle", 7, true},
        (wiconmap_t){"freezing_rain", "Freezing Rain", 8, true},
        (wiconmap_t){"freezing_rain_heavy", "Frezing Rain", 9, true},
        (wiconmap_t){"freezing_rain_light", "Freezing Rain", 10, true},
        (wiconmap_t){"ice_pellets", "Hail", 11, true},
        (wiconmap_t){"ice_pellets_heavy", "Hail", 12, true},
        (wiconmap_t){"ice_pellets_light", "Hail", 13, true},
        (wiconmap_t){"mostly_clear_day", "Mostly Clear", 14, true},
        (wiconmap_t){"mostly_clear_night", "Mostly Clear", 15, true},
        (wiconmap_t){"mostly_clear", "Mostly Clear", 14, true},
        (wiconmap_t){"partly_cloudy_day", "Partly Cloudy", 17, true},
        (wiconmap_t){"partly_cloudy_night", "Partly Cloudy", 18, true},
        (wiconmap_t){"partly_cloudy", "Partly Cloudy", 17, true},
        (wiconmap_t){"mostly_cloudy_day", "Mostly Cloudy", 16, true},
        (wiconmap_t){"mostly_cloudy_night", "Mostly Cloudy", 18, true},
        (wiconmap_t){"mostly_cloudy", "Mostly Cloudy", 16, true},
        (wiconmap_t){"rain", "Rain", 19, true},
        (wiconmap_t){"rain_heavy", "Heavy Rain", 20, true},
        (wiconmap_t){"rain_light", "Showers", 21, true},
        (wiconmap_t){"snow", "Snow", 22, true},
        (wiconmap_t){"snow_heavy", "Heavy Snow", 23, true},
        (wiconmap_t){"snow_light", "Snow Showers", 24, true},
        (wiconmap_t){"tstorm", "Thunderstorm", 25, true},
        (wiconmap_t){"???", "??not mapped??", 26, true},
    };

    int i;
    int l = (int)(sizeof(wmap) / sizeof(wmap[0]));
    // dictionary, hash - next pass...
    for (i = 0; i < l; i++) {
        //icon mapping inclusive day night modes
        if ((0 ==
             strcmp((const char *)wspecific, (const char *)wmap[i].code)) ||
            (0 == strcmp((const char *)key, (const char *)wmap[i].code))) {
            break;
        }
    }
    // pass mapped icon or fall through to unknown
    return wmap[i];
}

double calcSunEqOfCenter(double t);

double calcMeanObliquityOfEcliptic(double t) {
    double seconds = 21.448 - t * (46.8150 + t * (0.00059 - t * (0.001813)));
    double e0 = 23.0 + (26.0 + (seconds / 60.0)) / 60.0;
    return e0; // in degrees
}

double calcGeomMeanLongSun(double t) {
    double L = 280.46646 + t * (36000.76983 + 0.0003032 * t);
    while ((int)L > 360) {
        L -= 360.0;
    }
    while (L < 0) {
        L += 360.0;
    }
    return L; // in degrees
}

double calcObliquityCorrection(double t) {
    double e0 = calcMeanObliquityOfEcliptic(t);
    double omega = 125.04 - 1934.136 * t;
    double e = e0 + 0.00256 * cos(deg2Rad(omega));
    return e; // in degrees
}

double calcEccentricityEarthOrbit(double t) {
    double e = 0.016708634 - t * (0.000042037 + 0.0000001267 * t);
    return e; // unitless
}

double calcGeomMeanAnomalySun(double t) {
    double M = 357.52911 + t * (35999.05029 - 0.0001537 * t);
    return M; // in degrees
}

double calcEquationOfTime(double t) {
    double epsilon = calcObliquityCorrection(t);
    double l0 = calcGeomMeanLongSun(t);
    double e = calcEccentricityEarthOrbit(t);
    double m = calcGeomMeanAnomalySun(t);
    double y = tan(deg2Rad(epsilon) / 2.0);
    y *= y;
    double sin2l0 = sin(2.0 * deg2Rad(l0));
    double sinm = sin(deg2Rad(m));
    double cos2l0 = cos(2.0 * deg2Rad(l0));
    double sin4l0 = sin(4.0 * deg2Rad(l0));
    double sin2m = sin(2.0 * deg2Rad(m));
    double Etime = y * sin2l0 - 2.0 * e * sinm + 4.0 * e * y * sinm * cos2l0 -
                   0.5 * y * y * sin4l0 - 1.25 * e * e * sin2m;

    return rad2Deg(Etime) * 4.0; // in minutes of time
}

double calcTimeJulianCent(double jd) {
    double T = (jd - 2451545.0) / 36525.0;
    return T;
}

double calcSunTrueLong(double t) {
    double l0 = calcGeomMeanLongSun(t);
    double c = calcSunEqOfCenter(t);
    double O = l0 + c;
    return O; // in degrees
}

double calcSunApparentLong(double t) {
    double o = calcSunTrueLong(t);
    double omega = 125.04 - 1934.136 * t;
    double lambda = o - 0.00569 - 0.00478 * sin(deg2Rad(omega));
    return lambda; // in degrees
}

double calcSunDeclination(double t) {
    double e = calcObliquityCorrection(t);
    double lambda = calcSunApparentLong(t);
    double sint = sin(deg2Rad(e)) * sin(deg2Rad(lambda));
    double theta = rad2Deg(asin(sint));
    return theta; // in degrees
}

double calcHourAngleSunrise(double lat, double solarDec) {
    double latRad = deg2Rad(lat);
    double sdRad = deg2Rad(solarDec);
    double HA = (acos(cos(deg2Rad(90.833)) / (cos(latRad) * cos(sdRad)) -
                      tan(latRad) * tan(sdRad)));
    return HA; // in radians
}

double calcHourAngleSunset(double lat, double solarDec) {
    double latRad = deg2Rad(lat);
    double sdRad = deg2Rad(solarDec);
    double HA = (acos(cos(deg2Rad(90.833)) / (cos(latRad) * cos(sdRad)) -
                      tan(latRad) * tan(sdRad)));
    return -HA; // in radians
}

double calcJD(int year, int month, int day) {
    if (month <= 2) {
        year -= 1;
        month += 12;
    }
    int A = floor(year / 100);
    int B = 2 - A + floor(A / 4);

    double JD = floor(365.25 * (year + 4716)) + floor(30.6001 * (month + 1)) +
                day + B - 1524.5;
    return JD;
}

double calcJDFromJulianCent(double t) {
    double JD = t * 36525.0 + 2451545.0;
    return JD;
}

double calcSunEqOfCenter(double t) {
    double m = calcGeomMeanAnomalySun(t);
    double mrad = deg2Rad(m);
    double sinm = sin(mrad);
    double sin2m = sin(mrad + mrad);
    double sin3m = sin(mrad + mrad + mrad);
    double C = sinm * (1.914602 - t * (0.004817 + 0.000014 * t)) +
               sin2m * (0.019993 - 0.000101 * t) + sin3m * 0.000289;
    return C; // in degrees
}

double calcSunriseUTC(double JD, double latitude, double longitude) {
    double t = calcTimeJulianCent(JD);
    // *** First pass to approximate sunrise
    double eqTime = calcEquationOfTime(t);
    double solarDec = calcSunDeclination(t);
    double hourAngle = calcHourAngleSunrise(latitude, solarDec);
    double delta = longitude - rad2Deg(hourAngle);
    double timeDiff = 4 * delta;              // in minutes of time
    double timeUTC = 720 + timeDiff - eqTime; // in minutes
    double newt =
        calcTimeJulianCent(calcJDFromJulianCent(t) + timeUTC / 1440.0);

    eqTime = calcEquationOfTime(newt);
    solarDec = calcSunDeclination(newt);

    hourAngle = calcHourAngleSunrise(latitude, solarDec);
    delta = longitude - rad2Deg(hourAngle);
    timeDiff = 4 * delta;
    timeUTC = 720 + timeDiff - eqTime; // in minutes
    return timeUTC;
}

double calcSunsetUTC(double JD, double latitude, double longitude) {

    double t = calcTimeJulianCent(JD);
    // *** First pass to approximate sunset

    double eqTime = calcEquationOfTime(t);
    double solarDec = calcSunDeclination(t);
    double hourAngle = calcHourAngleSunset(latitude, solarDec);
    double delta = longitude - rad2Deg(hourAngle);
    double timeDiff = 4 * delta;              // in minutes of time
    double timeUTC = 720 + timeDiff - eqTime; // in minutes
    double newt =
        calcTimeJulianCent(calcJDFromJulianCent(t) + timeUTC / 1440.0);

    eqTime = calcEquationOfTime(newt);
    solarDec = calcSunDeclination(newt);

    hourAngle = calcHourAngleSunset(latitude, solarDec);
    delta = longitude - rad2Deg(hourAngle);
    timeDiff = 4 * delta;
    timeUTC = 720 + timeDiff - eqTime; // in minutes

    return timeUTC;
}

void brightnessEvent(void) {

    time_t now;
    time(&now);
    struct tm loctm = *localtime(&now);

    // capture for comparison
    time_t testsec = mktime(localtime(&now));

    bool is_am = (loctm.tm_hour < 12);

    // zero time to midnight
    loctm.tm_hour = 0;
    loctm.tm_min = 0;
    loctm.tm_sec = 0;

    char buffer[30];

    float JD = calcJD(loctm.tm_year + 1900, loctm.tm_mon + 1, loctm.tm_mday);

    double longitude = isp_locale.coords.Longitude;
    if (longitude < 0)
        longitude *= -1;

    char stb[BSIZE] = {0};

    time_t seconds = mktime(&loctm);
    struct tm *ptm = NULL;
    ptm = gmtime(&seconds);
    int delta = ptm->tm_hour; // TZ

    time_t tseconds = seconds;
    seconds = seconds +
              calcSunriseUTC(JD, isp_locale.coords.Latitude, longitude) * 60;
    seconds = seconds - delta * 3600;
    isp_locale.sunrise = seconds;

    if (isp_locale.brightness < 0) {
        strftime(buffer, 30, "%m-%d-%Y %T", localtime(&seconds));
        sprintf(stb, "%s %s\n", labelIt("Sunrise", LABEL_WIDTH, "."), buffer);
        putMSG(stb, LL_INFO);
    }

    seconds = tseconds;
    seconds += calcSunsetUTC(JD, isp_locale.coords.Latitude, longitude) * 60;
    seconds = seconds - delta * 3600;

    isp_locale.sunset = seconds;

    if (isp_locale.brightness < 0) {
        strftime(buffer, 30, "%m-%d-%Y %T", localtime(&seconds));
        sprintf(stb, "%s %s\n", labelIt("Sunset", LABEL_WIDTH, "."), buffer);
        putMSG(stb, LL_INFO);
    }

    double rsec = difftime(testsec, isp_locale.sunrise);
    double ssec = difftime(testsec, isp_locale.sunset);

    int current = isp_locale.brightness;
    if ((ssec > 0) && (rsec > 0)) {
        isp_locale.brightness = isp_locale.nightbright;
    } else if (rsec < 0) {
        isp_locale.brightness = isp_locale.nightbright;
    } else if ((rsec > 0) && (ssec < 0)) {
        isp_locale.brightness = isp_locale.daybright;
    }

    if (current != isp_locale.brightness) {
        if (isp_locale.brightness == isp_locale.nightbright) {
            daymode = false;
            sprintf(stb, "%s Night Mode\n",
                    labelIt("Set Display", LABEL_WIDTH, "."));
        } else {
            daymode = true;
            sprintf(stb, "%s Day Mode\n",
                    labelIt("Set Display", LABEL_WIDTH, "."));
        }
        putMSG(stb, LL_INFO);
        displayBrightness(isp_locale.brightness, true);
    }
}

void parseISP(char *jsonData, struct isp_locale_t *isp) {

    int v = getVerbose();
    jsmn_parser p;
    jsmntok_t jt[200];

    jsmn_init(&p);
    int r = jsmn_parse(&p, jsonData, strlen(jsonData), jt,
                       sizeof(jt) / sizeof(jt[0]));

    if (r < 0) {
        printf("[parseISP] Failed to parse JSON, check adequate tokens "
               "allocated: %d\n",
               r);
        if (v > LL_DEBUG) {
            printf("%s\n", jsonData);
        }
        return;
    }

    // Ensure the top-level element is an object
    if (r < 1 || jt[0].type != JSMN_OBJECT) {
        printf("[parseISP] Failed to parse JSON, non \"payload\", check "
               "adequate tokens "
               "allocated: %d\n",
               r);
        if (v > LL_DEBUG) {
            printf("%s\n", jsonData);
        }
        return;
    }

    char test[BSIZE];
    int done = 3;
    // Loop over all keys of the root object
    for (int i = 1; ((done > 0) || (i < r)); i++) {
        sprintf(test, "%.*s", jt[i + 1].end - jt[i + 1].start,
                jsonData + jt[i + 1].start);
        if (jsonEq(jsonData, &jt[i], "lon") == 0) {
            isp->coords.Longitude = strtod(test, NULL);
            i++;
            done--;
        } else if (jsonEq(jsonData, &jt[i], "lat") == 0) {
            isp->coords.Latitude = strtod(test, NULL);
            i++;
            done--;
        } else if (jsonEq(jsonData, &jt[i], "timezone") == 0) {
            strcpy(isp->Timezone, test);
            i++;
            done--;
        } else {
            continue; // unknown or we need to map
        }
    }
}

bool getLocation(void) {

    uint8_t port = 80;
    char host[BSIZE] = "bot.whatismyipaddress.com";
    char uri[BSIZE] = "/";
    char header[1] = "";
    char lookupIP[28] = {0};
    char stb[BSIZE] = {0};

    if (httpGet(host, port, uri, header, (char *)lookupIP)) {
        if (!isEmptyStr(lookupIP)) {

            sprintf(stb, "%s %s\n", labelIt("Provider IP", LABEL_WIDTH, "."),
                    lookupIP);
            putMSG(stb, LL_INFO);

            // now for provider details, inclusive lat/lon
            strcpy(host, "ip-api.com");
            sprintf(uri, "/json/%s", lookupIP);
            strcpy(header, "");
            char jsonData[BSIZE] = {0};

            if (httpGetNR(host, port, uri, header, (char *)jsonData)) {
                // parse/decompose and map json
                parseISP(jsonData, &isp_locale);

                sprintf(stb,
                        "%s %s\n"
                        "%s %9.4f\n"
                        "%s %9.4f\n",
                        labelIt("Reported TZ", LABEL_WIDTH, "."),
                        isp_locale.Timezone,
                        labelIt("Longitude", LABEL_WIDTH, "."),
                        isp_locale.coords.Longitude,
                        labelIt("Latitude", LABEL_WIDTH, "."),
                        isp_locale.coords.Latitude);

                putMSG(stb, LL_INFO);

                isp_locale.daybright = MAX_BRIGHTNESS;
                isp_locale.nightbright = NIGHT_BRIGHTNESS;
                isp_locale.brightness = -1;

                return true;
            }
        }
    }

    return false;
}

bool initAstral(void) {

    if (getLocation()) {
        brightnessEvent();
        return true;
    }
    return false;
}

// quick and dirty weather impl.
bool parseClimacell(char *jsonData, climacell_t *climacell) {

    jsmn_parser p;
    jsmntok_t jt[100];

    jsmn_init(&p);
    int r = jsmn_parse(&p, jsonData, strlen(jsonData), jt,
                       sizeof(jt) / sizeof(jt[0]));
    if (r < 0) {
        printf("Failed to parse JSON\nheck enough tokens allocated "
               "[parseClimacell]: %d\n",
               r);
        return false;
    }

    if (r < 1 || jt[0].type != JSMN_OBJECT) {
        printf("Object expected, [parseClimacell]\n");
        return false;
    }

    int done = 14;
    char lblit[32] = {0};

    // Loop over all keys of the root object
    for (int i = 1; ((done > 0) && (i < r)); i++) {

        jsmntok_t tok = jt[i];
        uint16_t lenk = tok.end - tok.start;
        char keyStr[lenk + 1];
        memcpy(keyStr, &jsonData[tok.start], lenk);
        keyStr[lenk] = '\0';
        sprintf(lblit, "weather:%s", keyStr);
        tok = jt[i + 1];
        uint16_t lenv = tok.end - tok.start;
        char valStr[lenv + 1];
        memcpy(valStr, &jsonData[tok.start], lenv);
        valStr[lenv] = '\0';

        ccdatum_t tstd;

        if (strncmp("lat", keyStr, 3) == 0) {
            climacell->coords.Latitude = strtod(valStr, NULL);
            if (getVerbose() >= LL_DEBUG)
                printf("%s %8.4f\n", labelIt(lblit, LABEL_WIDTH, "."),
                       climacell->coords.Latitude);
            done--;
            i++;
        } else if (strncmp("lon", keyStr, 3) == 0) {
            climacell->coords.Longitude = strtod(valStr, NULL);
            if (getVerbose() >= LL_DEBUG)
                printf("%s %8.4f\n", labelIt(lblit, LABEL_WIDTH, "."),
                       climacell->coords.Longitude);
            done--;
            i++;
        } else if (strncmp(climacell->temp.key, keyStr, climacell->temp.lenk) ==
                   0) {
            i++;
            if (jt[i].type == JSMN_OBJECT) {
                decodeKV(jsonData, &tstd, i + 1, i + 5, jt);
                if (copyCCDatum(&climacell->temp, &tstd)) {
                    if (getVerbose() >= LL_DEBUG)
                        printf("%s %5.2f %s\n",
                               labelIt(climacell->temp.lbl, LABEL_WIDTH, "."),
                               climacell->temp.fdatum, climacell->temp.units);
                }
                done--;
                i += 4; // close the group
            }
        } else if (strncmp(climacell->feels_like.key, keyStr,
                           climacell->feels_like.lenk) == 0) {
            i++;
            if (jt[i].type == JSMN_OBJECT) {
                decodeKV(jsonData, &tstd, i + 1, i + 5, jt);
                if (copyCCDatum(&climacell->feels_like, &tstd)) {
                    if (getVerbose() >= LL_DEBUG)
                        printf("%s %5.2f %s\n",
                               labelIt(climacell->feels_like.lbl, LABEL_WIDTH,
                                       "."),
                               climacell->feels_like.fdatum,
                               climacell->feels_like.units);
                }
                done--;
                i += 4; // close the group
            }
        } else if (strncmp(climacell->humidity.key, keyStr,
                           climacell->humidity.lenk) == 0) {
            i++;
            if (jt[i].type == JSMN_OBJECT) {
                decodeKV(jsonData, &tstd, i + 1, i + 5, jt);
                if (copyCCDatum(&climacell->humidity, &tstd)) {
                    if (getVerbose() >= LL_DEBUG)
                        printf(
                            "%s %5.2f %s\n",
                            labelIt(climacell->humidity.lbl, LABEL_WIDTH, "."),
                            climacell->humidity.fdatum,
                            climacell->humidity.units);
                }
                done--;
                i += 4; // close the group
            }
        } else if (strncmp(climacell->wind_speed.key, keyStr,
                           climacell->wind_speed.lenk) == 0) {
            i++;
            if (jt[i].type == JSMN_OBJECT) {
                decodeKV(jsonData, &tstd, i + 1, i + 5, jt);
                if (copyCCDatum(&climacell->wind_speed, &tstd)) {
                    if (getVerbose() >= LL_DEBUG)
                        printf("%s %5.2f %s\n",
                               labelIt(climacell->wind_speed.lbl, LABEL_WIDTH,
                                       "."),
                               climacell->wind_speed.fdatum,
                               climacell->wind_speed.units);
                }
                done--;
                i += 4; // close the group
            }
        } else if (strncmp(climacell->baro_pressure.key, keyStr,
                           climacell->baro_pressure.lenk) == 0) {
            i++;
            if (jt[i].type == JSMN_OBJECT) {
                decodeKV(jsonData, &tstd, i + 1, i + 5, jt);
                if (copyCCDatum(&climacell->baro_pressure, &tstd)) {
                    if (getVerbose() >= LL_DEBUG)
                        printf("%s %5.2f %s\n",
                               labelIt(climacell->baro_pressure.lbl,
                                       LABEL_WIDTH, "."),
                               climacell->baro_pressure.fdatum,
                               climacell->baro_pressure.units);
                }
                done--;
                i += 4; // close the group
            }
        } else if (strncmp(climacell->visibility.key, keyStr,
                           climacell->visibility.lenk) == 0) {
            i++;
            if (jt[i].type == JSMN_OBJECT) {
                decodeKV(jsonData, &tstd, i + 1, i + 5, jt);
                if (copyCCDatum(&climacell->visibility, &tstd)) {
                    if (getVerbose() >= LL_DEBUG)
                        printf("%s %5.2f %s\n",
                               labelIt(climacell->visibility.lbl, LABEL_WIDTH,
                                       "."),
                               climacell->visibility.fdatum,
                               climacell->visibility.units);
                }
                done--;
                i += 4; // close the group
            }
        } else if (strncmp(climacell->wind_direction.key, keyStr,
                           climacell->wind_direction.lenk) == 0) {
            i++;
            if (jt[i].type == JSMN_OBJECT) {
                decodeKV(jsonData, &tstd, i + 1, i + 5, jt);
                strcpy(tstd.sdatum, degToCompass(tstd.fdatum));
                if (0 !=
                    strcmp(tstd.sdatum, climacell->wind_direction.sdatum)) {
                    if (copyCCDatum(&climacell->wind_direction, &tstd))
                        if (getVerbose() >= LL_DEBUG)
                            printf("%s %3s : %5.2f %s\n",
                                   labelIt(climacell->wind_direction.lbl,
                                           LABEL_WIDTH, "."),
                                   climacell->wind_direction.sdatum,
                                   climacell->wind_direction.fdatum,
                                   climacell->wind_direction.units);
                }
                done--;
                i += 4; // close the group
            }
        } else if (strncmp(climacell->precipitation.key, keyStr,
                           climacell->precipitation.lenk) == 0) {
            i++;
            if (jt[i].type == JSMN_OBJECT) {
                decodeKV(jsonData, &tstd, i + 1, i + 5, jt);
                if (copyCCDatum(&climacell->precipitation, &tstd)) {
                    if (getVerbose() >= LL_DEBUG)
                        printf("%s %5.2f %s\n",
                               labelIt(climacell->precipitation.lbl,
                                       LABEL_WIDTH, "."),
                               climacell->precipitation.fdatum,
                               climacell->precipitation.units);
                }
                done--;
                i += 4; // close the group
            }
        } else if (strncmp("sunrise", keyStr, 7) == 0) {
            // skip
            done--;
            i += 3;
        } else if (strncmp("sunset", keyStr, 6) == 0) {
            // skip
            done--;
            i += 3;
        } else if (strncmp(climacell->weather_code.key, keyStr,
                           climacell->weather_code.lenk) == 0) {
            i++;
            if (jt[i].type == JSMN_OBJECT) {
                decodeKV(jsonData, &climacell->weather_code, i + 1, i + 3, jt);
                wiconmap_t tsti =
                    weatherIconXlate(climacell->weather_code.sdatum);
                if ((climacell->current.icon != tsti.icon) ||
                    (0 != strcmp(climacell->current.text, tsti.text)) ||
                    (climacell->weather_code.changed)) {
                    climacell->current = tsti; // sets changed flag too
                    if (getVerbose() >= LL_DEBUG)
                        printf("%s %s : %s (%d)\n",
                               labelIt(climacell->weather_code.lbl, LABEL_WIDTH,
                                       "."),
                               climacell->weather_code.sdatum,
                               climacell->current.text,
                               climacell->current.icon);
                }
                done--;
                i += 1; // close the group
            }
        } else if (strncmp(climacell->observation_time.key, keyStr,
                           climacell->observation_time.lenk) == 0) {
            i++;
            if (jt[i].type == JSMN_OBJECT) {
                decodeKV(jsonData, &climacell->observation_time, i + 1, i + 3,
                         jt);
                if (getVerbose() >= LL_DEBUG)
                    printf("%s %s\n",
                           labelIt(climacell->observation_time.lbl, LABEL_WIDTH,
                                   "."),
                           climacell->observation_time.sdatum);
                break; // always final kv
            }
        } else {
            continue; // unknown or we need to map
        }
    }

    return (done == 0);
}

void weatherEvent(struct climacell_t *climacell) { updClimacell(climacell); }

bool updClimacell(climacell_t *climacell) {

    uint16_t port = 443;
    char uri[BSIZE] = {0};
    char stb[BSIZE] = {0};

    if (isEmptyStr(climacell->host)) {
        strcpy(climacell->host, "api.climacell.co");
    }
    if (isEmptyStr(climacell->uri)) {
        strcpy(climacell->uri, "/v3/weather/realtime");
    }
    if (isEmptyStr(climacell->fields)) {
        strcpy(climacell->fields,
               "temp,feels_like,baro_pressure,visibility,humidity,"
               "precipitation,"
               "wind_speed,wind_direction,sunrise,sunset,weather_code");
    }
    if (isEmptyStr(climacell->units)) {
        strcpy(climacell->units, "us");
    }

    if (0 == climacell->coords.Latitude) {
        if (getLocation()) {
            climacell->coords.Longitude = isp_locale.coords.Longitude;
            climacell->coords.Latitude = isp_locale.coords.Latitude;
        } else {
            // unknown - no-go - try later
            return false;
        }
    }

    sprintf(uri, "%s?apikey=%s&lat=%f&lon=%f&unit_system=%s", climacell->uri,
            climacell->Apikey, climacell->coords.Latitude,
            climacell->coords.Longitude, climacell->units);

    // strtok is destructive, make a scratch copy
    char *ccf = (char *)malloc(sizeof(char) * (strlen(climacell->fields) + 1));
    strcpy(ccf, climacell->fields);
    char *field = strtok(ccf, ",");
    bool delim = false;
    while (field) {
        if (delim)
            strcat(uri, "%2C");
        else
            strcat(uri, "&fields=");
        strcat(uri, field);
        field = strtok(NULL, ",");
        delim = true;
    }
    if (ccf)
        free(ccf);

    BIO *bio;
    SSL_CTX *ctx;
    char *body;

    SSL_library_init();
    ctx = SSL_CTX_new(SSLv23_client_method());

    if (ctx == NULL) {
        printf("SSL ctx is NULL\n");
        return false;
    }

    char host[156] = {0};
    sprintf(host, "%s:%d", climacell->host, port);

    bio = BIO_new_ssl_connect(ctx);
    BIO_set_conn_hostname(bio, host);

    if (BIO_do_connect(bio) <= 0) {
        printf("Failed Climacell connection\n");
        if (bio)
            BIO_free_all(bio);
        if (ctx)
            SSL_CTX_free(ctx);
        return false;
    }

    char *write_buf;
    int r = asprintf(&write_buf,
                     "%s %s HTTP/1.1\r\n"
                     "Host: %s\r\n"
                     "Accept: application/json\r\n"
                     "Content-Type: application/json\r\n"
                     "Connection: close\r\n"
                     "\r\n",
                     (char *)httpMethodString(GET), uri, climacell->host);

    if (0 == r) {
        if (bio)
            BIO_free_all(bio);
        if (ctx)
            SSL_CTX_free(ctx);
        return 0;
    }

    if (BIO_write(bio, write_buf, strlen(write_buf)) <= 0) {
        printf("Failed write\n");
        if (bio)
            BIO_free_all(bio); // boy I miss defer
        if (ctx)
            SSL_CTX_free(ctx);
        return false;
    }

    if (write_buf)
        free(write_buf);

    int size;
    char buf[4096];

    for (;;) {
        size = BIO_read(bio, buf, 4095);
        if (size <= 0)
            break;
        buf[size] = 0;

        if (strstr(buf, "HTTP/1.1 200 OK") == NULL) {
            if (bio)
                BIO_free_all(bio);
            if (ctx)
                SSL_CTX_free(ctx);
            return 0;
        }

        const char *p1 = strstr(buf, "Content-Length: ") + 16;
        const char *p2 = strstr(p1, "\r\n");
        size_t len = p2 - p1;
        char *ptr;
        char *res = (char *)malloc(sizeof(char) * (len + 1));
        strncpy(res, p1, len);
        res[len] = '\0';
        body = strstr(buf, "\r\n\r\n") + 4; // body
        int zz = strlen(body);
        int zt = strtol(res, &ptr, 10);
        if (zz == zt) {
            if (res)
                free(res);
            climacell->refreshed = parseClimacell(body, climacell);
            break;
        }
        if (res)
            free(res);
    }

    if (bio)
        BIO_free_all(bio);
    if (ctx)
        SSL_CTX_free(ctx);

    return true;
}
