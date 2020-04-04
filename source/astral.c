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

#include "astral.h"
#include "common.h"
#ifndef JSMN_STATIC
#define JSMN_STATIC
#endif
#include "../source/jsmn.h"

#include "display.h"

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif

isp_locale_t isp_locale;

static int jsonEq(const char *json, jsmntok_t *tok, const char *s) {
    if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
        strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
        return 0;
    }
    return -1;
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

	time_t now;	time(&now);
	struct tm loctm = *localtime(&now);

    // capture for comparison
    time_t testsec = mktime(localtime(&now));

    bool is_am = (loctm.tm_hour < 12);

    // zero time to midnight
    loctm.tm_hour = 0;
    loctm.tm_min = 0;
    loctm.tm_sec = 0;

    char buffer[30];

    float JD = calcJD(loctm.tm_year+1900, loctm.tm_mon+1, loctm.tm_mday);

    double longitude = isp_locale.Longitude;
    if (longitude < 0)
        longitude *= -1;

    char stb[BSIZE] = {0};

    time_t seconds = mktime(&loctm);
    struct tm *ptm = NULL;
    ptm = gmtime(&seconds); 
    int delta = ptm->tm_hour; // TZ

    time_t tseconds = seconds;
    seconds = seconds + calcSunriseUTC(JD, isp_locale.Latitude, longitude) * 60;
    seconds = seconds - delta * 3600;
    isp_locale.sunrise = seconds;

    if (isp_locale.brightness < 0)
    {
        strftime(buffer, 30, "%m-%d-%Y %T", localtime(&seconds));
        sprintf(stb, "Sunrise ........: %s\n", buffer);
        putMSG(stb, LL_INFO);
    }

    seconds = tseconds;
    seconds += calcSunsetUTC(JD, isp_locale.Latitude, longitude) * 60;
    seconds = seconds - delta * 3600;

    isp_locale.sunset = seconds;

    if (isp_locale.brightness < 0)
    {
        strftime(buffer, 30, "%m-%d-%Y %T", localtime(&seconds));
        sprintf(stb, "Sunset .........: %s\n", buffer);
        putMSG(stb, LL_INFO);
    }

    double rsec = difftime(testsec, isp_locale.sunrise);
    double ssec = difftime(testsec, isp_locale.sunset);

    //if (isp_locale.brightness < 0)
    //{
    //    sprintf(stb, "rsec (%f) ssec (%f)\n", rsec, ssec);
    //    putMSG(stb, LL_INFO);
    //}

    int current = isp_locale.brightness;
    if ((ssec > 0)&&(rsec > 0))
    {
        isp_locale.brightness = isp_locale.nightbright;
    }
    else if (rsec < 0)
    {
        isp_locale.brightness = isp_locale.nightbright;
    }
    else if ((rsec > 0)&&(ssec < 0))
    {
        isp_locale.brightness = isp_locale.daybright;
    }

    if (current != isp_locale.brightness)
    {
        if (isp_locale.brightness == isp_locale.nightbright)
            putMSG("Set Display ....: Night Mode\n", LL_INFO);
        else
            putMSG("Set Display ....: Day Mode\n", LL_INFO);
        displayBrightness(isp_locale.brightness);
    }
}

struct Request *parse_request(const char *raw) {
    struct Request *req = NULL;
    req = malloc(sizeof(struct Request));
    if (!req) {
        return NULL;
    }
    memset(req, 0, sizeof(struct Request));

    // Method
    size_t meth_len = strcspn(raw, " ");
    if (memcmp(raw, "GET", strlen("GET")) == 0) {
        req->method = GET;
    } else if (memcmp(raw, "POST", strlen("POST")) == 0) {
        req->method = POST;
    } else if (memcmp(raw, "HEAD", strlen("HEAD")) == 0) {
        req->method = HEAD;
    } else {
        req->method = UNSUPPORTED;
    }
    raw += meth_len + 1; // move past <SP>

    // Request-URI
    size_t url_len = strcspn(raw, " ");
    req->url = malloc(url_len + 1);
    if (!req->url) {
        free_request(req);
        return NULL;
    }
    memcpy(req->url, raw, url_len);
    req->url[url_len] = '\0';
    raw += url_len + 1; // move past <SP>

    // HTTP-Version
    size_t ver_len = strcspn(raw, "\r\n");
    req->version = malloc(ver_len + 1);
    if (!req->version) {
        free_request(req);
        return NULL;
    }
    memcpy(req->version, raw, ver_len);
    req->version[ver_len] = '\0';
    raw += ver_len + 2; // move past <CR><LF>

    struct Header *header = NULL, *last = NULL;
    while (raw[0] != '\r' || raw[1] != '\n') {
        last = header;
        header = malloc(sizeof(Header));
        if (!header) {
            free_request(req);
            return NULL;
        }

        // name
        size_t name_len = strcspn(raw, ":");
        header->name = malloc(name_len + 1);
        if (!header->name) {
            free_request(req);
            return NULL;
        }
        memcpy(header->name, raw, name_len);
        header->name[name_len] = '\0';
        raw += name_len + 1; // move past :
        while (*raw == ' ') {
            raw++;
        }

        // value
        size_t value_len = strcspn(raw, "\r\n");
        header->value = malloc(value_len + 1);
        if (!header->value) {
            free_request(req);
            return NULL;
        }
        memcpy(header->value, raw, value_len);
        header->value[value_len] = '\0';
        raw += value_len + 2; // move past <CR><LF>

        // next
        header->next = last;
    }
    req->headers = header;
    raw += 2; // move past <CR><LF>

    size_t body_len = strlen(raw);
    req->body = malloc(body_len + 1);
    if (!req->body) {
        free_request(req);
        return NULL;
    }
    memcpy(req->body, raw, body_len);
    req->body[body_len] = '\0';

    return req;
}

void free_header(struct Header *h) {
    if (h) {
        free(h->name);
        free(h->value);
        free_header(h->next);
        free(h);
    }
}

void free_request(struct Request *req) {
    free(req->url);
    free(req->version);
    free_header(req->headers);
    free(req->body);
    free(req);
}

void parseISP(char *data, struct isp_locale_t *isp) {

    int i;
    int r;
    jsmn_parser p;
    jsmntok_t t[30]; // Expect no more than 30 tokens

    char test[1024];

    jsmn_init(&p);
    r = jsmn_parse(&p, data, strlen(data), t, sizeof(t) / sizeof(t[0]));
    if (r < 0) {
        return;
    }

    /* Assume the top-level element is an object */
    if (r < 1 || t[0].type != JSMN_OBJECT) {
        return;
    }

    // Loop over all keys of the root object
    for (i = 1; i < r; i++) {
        sprintf(test, "%.*s", t[i + 1].end - t[i + 1].start,
                data + t[i + 1].start);
        if (jsonEq(data, &t[i], "lon") == 0) {
            isp->Longitude = strtod(test, NULL);
            i++;
        } else if (jsonEq(data, &t[i], "lat") == 0) {
            isp->Latitude = strtod(test, NULL);
            i++;
        } else if (jsonEq(data, &t[i], "timezone") == 0) {
            strcpy(isp->Timezone, test);
            i++;
        } else {
            continue; // unknown or we need to map
        }
    }
}

bool http_get(char *host, uint8_t port, char *uri, char *response) {

    char buffer[BUFSIZ];
    enum CONSTEXPR { MAX_REQUEST_LEN = 1024 };
    char request[MAX_REQUEST_LEN];

    char request_template[] = "GET %s HTTP/1.1\r\nHost: %s\r\n\r\n";
    struct protoent *protoent;
    in_addr_t in_addr;
    int sockFD;
    struct hostent *hostent;
    struct sockaddr_in sockaddr_in;
    ssize_t req_len, nbytes_total;

    req_len = snprintf(request, MAX_REQUEST_LEN, request_template, uri, host);
    if (req_len >= MAX_REQUEST_LEN) {
        fprintf(stderr, "request length overrun: %ld\n", (long)req_len);
        return false;
    }

    // open the socket.
    protoent = getprotobyname("tcp");
    if (protoent == NULL) {
        fprintf(stderr, "error on getprotobyname(\"tcp\")\n");
        return false;
    }
    sockFD = socket(AF_INET, SOCK_STREAM, protoent->p_proto);
    if (sockFD == -1) {
        fprintf(stderr, "bad socket descriptor\n");
        return false;
    }

    // flesh the address
    hostent = gethostbyname(host);
    if (hostent == NULL) {
        fprintf(stderr, "error: gethostbyname(\"%s\")\n", host);
        return false;
    }
    in_addr = inet_addr(inet_ntoa(*(struct in_addr *)*(hostent->h_addr_list)));
    if (in_addr == (in_addr_t)-1) {
        fprintf(stderr, "error: inet_addr(\"%s\")\n", *(hostent->h_addr_list));
        return false;
    }
    sockaddr_in.sin_addr.s_addr = in_addr;
    sockaddr_in.sin_family = AF_INET;
    sockaddr_in.sin_port = htons(port);

    // And connect ...
    if (connect(sockFD, (struct sockaddr *)&sockaddr_in, sizeof(sockaddr_in)) ==
        -1) {
        fprintf(stderr, "error: connect - %s\n", host);
        return false;
    }

    // Send HTTP request.
    if (write(sockFD, request, strlen(request)) < 0) {
        fprintf(stderr, "ERROR writing to socket!");
        return false;
    }
    if ((nbytes_total = read(sockFD, buffer, BUFSIZ - 1)) < 0) {
        fprintf(stderr, "ERROR reading from socket!");
        return false;
    }
    buffer[nbytes_total] = 0;

    close(sockFD);

    if (nbytes_total == -1) {
        fprintf(stderr, "error: bad request - %s\n", host);
        return false;
    }

    char *data = strstr(buffer, "200 OK");
    if (data == NULL)
        return false;
    else
        data += 7;

    data = strstr(data, "\r\n\r\n");
    if (data != NULL) {
        data += 4;
        strcpy(response, data);
    } else
        return false;

    return true;
}

bool initAstral(void) {

    uint8_t port = 80;
    char host[BSIZE] = {0};
    char uri[BSIZE] = {0};
    char lookupIP[20] = {0};
    char stb[BSIZE] = {0};

    // get ISP IP
    strcpy(host, "bot.whatismyipaddress.com");
    strcpy(uri, "/");

    if (http_get(host, port, uri, (char *)lookupIP)) {
        if (!isEmptyStr(lookupIP)) {
            sprintf(stb, "Provider IP ....: %s\n", lookupIP);
            putMSG(stb, LL_INFO);

            // now for provider details, inclusive lat/lon
            strcpy(host, "ip-api.com");
            sprintf(uri, "/json/%s", lookupIP);
            char jsonData[4096] = {0};

            if (http_get(host, port, uri, (char *)jsonData)) {
                // parse
                parseISP(jsonData, &isp_locale);
                sprintf(stb,
                        "Reported TZ ....: %s\n"
                        "Longitude ......: %9.4f\n"
                        "Latitude .......: %9.4f\n",
                        isp_locale.Timezone, isp_locale.Longitude,
                        isp_locale.Latitude);
                putMSG(stb, LL_INFO);

                isp_locale.daybright = MAX_BRIGHTNESS;
                isp_locale.nightbright = NIGHT_BRIGHTNESS;
                isp_locale.brightness = -1;

                brightnessEvent();
            }
            return true;
        }
    }

    return false;
}
