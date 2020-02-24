/*
 *	Tools to handle tagged Slimserver output
 *
 * 	(c) 2015 László TÓTH
 *	(c) 2020 Stuart Hunter
 *
 * 	This program is free software: you can redistribute it and/or modify
 * 	it under the terms of the GNU General Public License as published by
 * 	the Free Software Foundation, either version 3 of the License, or
 * 	(at your option) any later version.
 *
 * 	This program is distributed in the hope that it will be useful,
 * 	but WITHOUT ANY WARRANTY; without even the implied warranty of
 * 	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * 	GNU General Public License for more details.
 *
 * 	See <http://www.gnu.org/licenses/> to get a copy of the GNU General
 * 	Public License.
 *
 */

#include <ctype.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "sliminfo.h"

#define MAXTAGLEN 255

/*
 *  Based of: URL decoding function from
 * http://rosettacode.org/wiki/URL_encoding
 *
 * 	László TÓTH: Build character tables once - on the first call.
 */
char rfc3986[256] = {0};
// char html5[256]   = {0};

void buildChrTabs(void) {
  for (int i = 0; i < 256; i++) {
    rfc3986[i] =
        isalnum(i) || i == '~' || i == '-' || i == '.' || i == '_' ? i : 0;
    //		html5[i]   = isalnum(i)||i == '*'||i == '-'||i == '.'||i == '_' ? i :
    //(i == ' ') ? '+' : 0;
  }
}

void encode(const char *s, char *enc) {
  if (rfc3986[0] == 0) {
    buildChrTabs();
  }

  for (; *s; s++) {
    if (rfc3986[(unsigned char)*s]) {
      sprintf(enc, "%c", rfc3986[(unsigned char)*s]);
    } else {
      sprintf(enc, "%%%02X", *s);
    }
    while (*++enc)
      ;
  }
}

/*
 *  Based of: URL decoding function from
 * http://rosettacode.org/wiki/URL_decoding
 *
 * 	László TÓTH: Stop decoding when space or \n terminate the tag.
 *               Added basic UTF-8 / ASCII conversion to most important
 * characters
 */
inline int ishex(int x) {
  return (x >= '0' && x <= '9') || (x >= 'a' && x <= 'f') ||
         (x >= 'A' && x <= 'F');
}

char getASCII(int utf8) {
  char ascii = '.';
  // 0x00C0
  char mainMap[] = "AAAAAAACEEEEIIIIDNOOOOOx0UUUUY.B"
                   "aaaaaaaceeeeiiiionooooo-ouuuuy.y"
                   "AaAaAaCcCcCcCcDdDdEeEeEeEeEeGgGg"
                   "GgGgHhHhIiIiIiIiIiIiJjKkkLlLlLlL"
                   "lLlNnNnNnnnnOoOoOo..RrRrRrSsSsSs"
                   "SsTtTtFfUuUuUuUuUuUuWwYyYZzZzZzf"
                   "bB...............FfG...IKkl..N.0"
                   "................................"
                   "|..!.........AaIiOoUuUuUuUuUueAa"
                   "AaAaGgGgKkQqQq33J...GgHpNnAaAa00"
                   "AaAaEeEeIiIiOoOoRrRrUuUuSsTt33Hh"
                   "nd..ZzAaEeOoOoOoOoYylnrj..ACcLTs"; // 0x022F

  int extraUtfMap[] = {225, 0};
  char extraChrMap[] = {'a', '.'};
  int i;

  if ((utf8 >= 0x00C0) && (utf8 <= 0x022F)) {
    ascii = mainMap[utf8 - 0x00C0];
  } else {
    for (i = 0; (extraUtfMap[i] != utf8) && (extraUtfMap[i] != 0); i++) {
      if (extraUtfMap[i] == 0) {
        printf("Unhandled UTF-8 code! %d\n", utf8);
      }
      ascii = extraChrMap[i];
    }
  }
  return ascii;
}

int decode(const char *s, char *dec) {
  char *o;
  const char *end;
  int c;
  int utfC = 0;
  int mb = 0;

  if ((s == NULL) && (dec == NULL)) {
    return -1;
  }

  if ((end = strchr(s, ' ')) == NULL) {
    if ((end = strchr(s, '\n')) == NULL) {
      end = s + strlen(s);
    }
  }
  end--;

  for (o = dec; s <= end;) {
    c = *s++;
    if (c == '+')
      c = ' ';
    else if (c == '%' &&
             (!ishex(*s++) || !ishex(*s++) || !sscanf(s - 2, "%2x", &c)))
      return -1;

    if ((c & 0b10000000) != 0) {
      // UTF-8 handling
      if ((c & 0b01000000) != 0) {
        // the first byte of UTF-8 character)
        utfC = c & 0b00111111;
        char mask = 0b00100000;
        mb = 1;
        while ((c & mask) != 0) {
          utfC = utfC & (!mask);
          mask = mask >> 1;
          mb++;
        }
      } else {
        // next bytes of UTF-8 character
        utfC = (utfC << 6) | (c & 0b00111111);
        if (--mb == 0) {
          *(o++) = getASCII(utfC);
        }
      }
    } else {
      *(o++) = c;
    }
  }

  *o = 0;

  return o - dec;
}
/***********************************************************************/

char *getTag(const char *tag, char *input, char *output, int outSize) {
  char exactTag[MAXTAGLEN];
  char *foundT;
  char *lastCHR;
  int tagLength;

  if ((tag == NULL) || (input == NULL) || (output == NULL)) {
    return NULL;
  }

  sprintf(exactTag, " %s%%3A", tag);

  if ((foundT = strstr(input, exactTag)) == NULL) {
    return NULL;
  }

  foundT = strstr(foundT, "%3A") + (3 * sizeof(char));
  if ((lastCHR = strchr(foundT, ' ')) != NULL) {
    tagLength = lastCHR - foundT;
  } else {
    tagLength = strlen(foundT);
  }

  if (tagLength < outSize) {
    decode(foundT, output);
  }

  return output;
}

char *getQuality(char *input, char *output, int outSize) {
  long sampleSize;
  long sampleRate;
  char tagData[MAXTAGLEN];

  if ((input == NULL) || (output == NULL)) {
    return NULL;
  }

  if (outSize < (7 * (int)sizeof(char))) {
    return NULL;
  }
  if ((getTag("samplesize", input, tagData, MAXTAGLEN)) == NULL) {
    return NULL;
  }
  if ((sampleSize = strtol(tagData, NULL, 10)) == 0) {
    return NULL;
  }
  if ((getTag("samplerate", input, tagData, MAXTAGLEN)) == NULL) {
    return NULL;
  }
  if ((sampleRate = strtol(tagData, NULL, 10)) == 0) {
    return NULL;
  }

  sprintf(output, "%ld/%ld", sampleSize, sampleRate / 1000);

  return output;
}

int isPlaying(char *input) {
  char tagData[MAXTAGLEN];

  if (input == NULL) {
    return true;
  }

  if ((getTag("mode", input, tagData, MAXTAGLEN)) == NULL) {
    return true;
  }
  if (strstr("play", tagData) != NULL) {
    return true;
  }

  return false;
}

long getMinute(tag *timeTag) {

  if (timeTag == NULL) {
    return 0;
  }
  if (!timeTag->valid) {
    return 0;
  }

  return strtol(timeTag->tagData, NULL, 10);
}
