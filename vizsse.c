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

#include "visdata.h"
#include "visualize.h"
#include "vizsse.h"
#include "log.h"

pthread_t vizSSEThread;

static void parse_arguments(int argc, char** argv);

static size_t on_data(char *ptr, size_t size, size_t nmemb, void *userdata)
{  
  parse_payload((const char*)userdata);
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

void parse_payload(const char* data)
{
  printf("the data >>>>>>>>>>>>>>>>>>>\n%s\n",data);
  // TODO: implementation details
}

void on_sse_event(char** headers, const char* data)
{
    // parse mesage payload - simple JSON - keep it light-weight
    parse_payload(data);
    visualize(); // go visualize (if screen active)

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
