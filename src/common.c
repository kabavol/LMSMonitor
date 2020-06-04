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

#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <sys/time.h>
#include <time.h>

#include "common.h"

int verbose = 0;
int textOut = false;

void dodelay(uint16_t d) { usleep(d * 1000); }

char *strrep(size_t n, const char *s) {

    size_t slen = strlen(s);
    char *dest = malloc(n * slen + 1);

    char *p = dest;
    for (int i = 0; i < n; ++i, p += slen) {
        memcpy(p, s, slen);
    }
    *p = '\0';
    return dest;
}

char *labelIt(const char *label, const size_t len, const char *pad) {

    size_t l = strlen(label);
    if (l > len) {
        return (char *)label;
    } else {
        char *dest = malloc(len + 1);
        sprintf(dest, "%s %s:", label, strrep((len - l) - 2, pad));
        return dest;
    }
}

int incVerbose(void) {
    if (verbose < LL_MAX - 1) {
        verbose++;
    }
    return verbose;
}

int getVerbose(void) { return verbose; }
const char *getVerboseStr(void) {
    if (verbose < LL_MAX)
        return verbosity[verbose];
    else
        return "Unknown";
}

bool debugActive(void) { return (verbose >= LL_DEBUG); }

int putMSG(const char *msg, int loglevel) {
    if (loglevel > verbose) {
        return false;
    }
    printf("%s", msg);
    return true;
}

void enableTOut(void) { textOut = true; }

int tOut(const char *msg) {
    if (!textOut) {
        return false;
    }
    printf("%s", msg);
    return true;
}

void abortMonitor(const char *msg) {
    perror(msg);
    exit(1);
}

bool isEmptyStr(const char *s) {
    if ((s == NULL) || (s[0] == '\0'))
        return true;
    return false;
}

char *replaceStr(const char *s, const char *find, const char *replace) {
    char *result;
    int i, cnt = 0;
    int rlen = strlen(replace);
    int flen = strlen(find);

    // Counting the number of times substring
    // occurs in the string
    for (i = 0; s[i] != '\0'; i++) {
        if (strstr(&s[i], find) == &s[i]) {
            cnt++;
            i += flen - 1;
        }
    }
    // Making new string of enough length
    result = (char *)malloc(i + cnt * (rlen - flen) + 1);

    i = 0;
    while (*s) {
        // compare the substring with the result
        if (strstr(s, find) == s) {
            strcpy(&result[i], replace);
            i += rlen;
            s += flen;
        } else
            result[i++] = *s++;
    }

    result[i] = '\0';
    return result;
}

char *multi_tok(char *input, multi_tok_t *string, char *delimiter) {
    if (input != NULL)
        *string = input;

    if (*string == NULL)
        return *string;

    char *end = strstr(*string, delimiter);
    if (end == NULL) {
        char *temp = *string;
        *string = NULL;
        return temp;
    }

    char *temp = *string;

    *end = '\0';
    *string = end + strlen(delimiter);
    return temp;
}

multi_tok_t multi_tok_init() { return NULL; }

#include <ctype.h>
// missing under linux
void strupr(char *s) {
    char *p = s;
    while (*p) {
        *p = toupper(*p);
        ++p;
    }
}

int piVersion(void) {
    int ret = 2;
    ////cat /sys/firmware/devicetree/base/model
    return ret;
}

void sinisterRotate(char *rm) {
    int n = strlen(rm);
    uint8_t temp = rm[0];
    int i;
    for (i = 0; i < n - 1; i++)
        rm[i] = rm[i + 1];
    rm[i] = temp;
}

void dexterRotate(char *rm) {
    int n = strlen(rm);
    uint8_t temp = rm[n - 1];
    int i;
    for (i = n - 1; i > 0; i--)
        rm[i] = rm[i - 1];
    rm[0] = temp;
}

void instrument(const int line, const char *name, const char *msg) {
    if (debugActive()) {
        char tbuff[32];
        char buff[BSIZE];
        struct timeval tv;
        gettimeofday(&tv, NULL);
        time_t now = tv.tv_sec;
        struct tm loctm = *localtime(&now);
        strftime(tbuff, sizeof(tbuff), "%Y-%m-%02d %H:%M:%S", &loctm);
        sprintf(buff, "%19s :: %16s-%04d : %s\n", tbuff, name, line, msg);
        putMSG(buff, LL_DEBUG);
    }
}
