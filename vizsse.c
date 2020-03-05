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

#include <pthread.h>

#include <stdbool.h>
#include <regex.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include <sys/time.h>
#include <time.h>

#include "http.h"

#include "log.h"
#include "jsmn.h"

#include "visualize.h"
#include "vizsse.h"

pthread_t vizSSEThread;

static size_t on_data(char *ptr, size_t size, size_t nmemb, void *userdata)
{  
  parse_payload(ptr, size * nmemb);
  return size * nmemb;
} 

static const char* verify_sse_response(CURL* curl) {
  #define EXPECTED_CONTENT_TYPE "text/event-stream"
  
  static const char expected_content_type[] = EXPECTED_CONTENT_TYPE;

  char* content_type;
  curl_easy_getinfo(curl, CURLINFO_CONTENT_TYPE, &content_type); 
  if(!content_type) content_type = 0;

  if(!strncmp(content_type, expected_content_type, strlen(expected_content_type)))
    return 0;
  
  return "Invalid content_type, should be '" EXPECTED_CONTENT_TYPE "'.";
}

void on_sse_event(char** headers, const char* data)
{
    char* result = 0;
    // parse mesage payload - it is simple JSON - keep it light-weight

    struct vissy_meter_t vissy_meter = {
        .channel_name = {"L", "R"},
        .is_mono = 0,
        .sample_accum = {0, 0},
        .floor = -96,
        .reference = 32768,
        .dBfs = {-1000,-1000},
        .dB = {-1000,-1000},
        .linear = {0,0},
        .rms_bar = {0,0},
        .channel_width = {192, 192},
        .bar_size = {6, 6},
        .channel_flipped = {0, 0},
        .clip_subbands = {0, 0},
        .sample_bin_chan = {
          {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        },
          {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
        }},
    };

    parse_visualize((char*)data, &vissy_meter);
    visualize(&vissy_meter); // go visualize (if screen active)

    free(result);

}

void *vizSSEPolling(void *x_voidptr) {
  const char* headers[] = {
    "Accept: text/event-stream",
    "Cache-Control: no-cache",
    "Connection: keep-alive",
    NULL
  };
  // calls libcurl
  http(HTTP_GET, options.uri, headers, 0, 0, on_data, verify_sse_response);
  return 0;

}

static void parseSSEArguments(int argc, char** argv)
{
  // set default url and allow_insecure is always set to true for now
  options.allow_insecure = 1;
  options.uri = "http://192.168.1.37:8022/visionon?subscribe=SA-VU";
    
  if(!options.allow_insecure) {
    if(strncmp(options.uri, "https:", 6)) {
      fprintf(stderr, "Insecure connections not allowed, use -i, if necessary.\n");
      exit(1);
    }
  } 
}

bool setupSSE(int argc, char** argv) 
{

    bool ret = true;
    // pass in arguments that will be used in REST call/connection
    parseSSEArguments(argc, argv);

    if (pthread_create(&vizSSEThread, NULL, vizSSEPolling, NULL) != 0) {
        vissySSEFinalize();
        toLog(1, "Failed to create SSE Visualization thread!");
        ret = false;
    }
    return ret;

}

void vissySSEFinalize(void)
{
    pthread_cancel(vizSSEThread);
    pthread_join(vizSSEThread, NULL);  
}

static int jsoneq(const char *json, jsmntok_t *tok, const char *s) {
  if (tok->type == JSMN_STRING && (int)strlen(s) == tok->end - tok->start &&
      strncmp(json + tok->start, s, tok->end - tok->start) == 0) {
    return 0;
  }
  return -1;
}

void parse_visualize(char *data, struct vissy_meter_t *vissy_meter)
{

  int i;
  int r;
  jsmn_parser p;
  jsmntok_t t[128]; /* We expect no more than 128 tokens */

    uint8_t channel = 0;
    bool is_vu = true;
    char test[1024];
    int numFFT = 0;

  jsmn_init(&p);
  r = jsmn_parse(&p, data, strlen(data), t,
                 sizeof(t) / sizeof(t[0]));
  if (r < 0) {
    // buffer shunting ???
    //printf("Failed to parse JSON: %d\n", r);
    return;
  }

  /* Assume the top-level element is an object */
  if (r < 1 || t[0].type != JSMN_OBJECT) {
    //printf("Object expected\n");
    return;
  }

    /* Loop over all keys of the root object */
  for (i = 1; i < r; i++) {
    sprintf(test, "%.*s", t[i + 1].end - t[i + 1].start,
            data + t[i + 1].start);
    if (jsoneq(data, &t[i], "type") == 0) {
      /* We may use strndup() to fetch string value */
      is_vu = (strncmp(test, "VU", 2));
      i++;
    } else if (jsoneq(data, &t[i], "channel") == 0) {
      /* We may additionally check if the value is either "true" or "false" */
      // ignore - we'll digest
      i++;
    } else if (jsoneq(data, &t[i], "name") == 0) {
      channel = (strncmp(test, "L", 1)) ? 0 : 1;
      i++;
    } else if (jsoneq(data, &t[i], "accumulated") == 0) {
      vissy_meter->sample_accum[channel] = atoi(test);
      i++;
    } else if (jsoneq(data, &t[i], "scaled") == 0) {
      vissy_meter->rms_scale[channel] = atoi(test);
      i++;
    } else if (jsoneq(data, &t[i], "dBfs") == 0) {
      vissy_meter->dBfs[channel] = atoi(test);
      i++;
    } else if (jsoneq(data, &t[i], "dB") == 0) {
      vissy_meter->dB[channel] = atoi(test);
      i++;
    } else if (jsoneq(data, &t[i], "linear") == 0) {
      vissy_meter->linear[channel] = atoi(test);
      i++;
    } else if (jsoneq(data, &t[i], "numFFT") == 0) {
      numFFT = atoi(test);
      i++;
    } else if (jsoneq(data, &t[i], "FFT") == 0) {
      int j;
      if (t[i + 1].type != JSMN_ARRAY) {
        continue; // We expect FFT values to be an array of int
      }
      for (j = 0; j < t[i + 1].size; j++) {
        jsmntok_t *g = &t[i + j + 2];
        sprintf(test,"%.*s", g->end - g->start, data + g->start);
        vissy_meter->sample_bin_chan[channel][j] = atoi(test);
      }
      i += t[i + 1].size + 1;
    } else {
      continue; // unknown or need to map
    }
  }

}
