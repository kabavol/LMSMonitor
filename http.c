/*
 *	(c) 2020 Stuart Hunter
 *
 *	TODO:
 *    Eliminate CURL requirement
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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include "http.h"
#include "vizsse.h"

#define SSE_CLIENT_USERAGENT     "LMSMonitor/" VERSION

#define die(msg) do { perror(msg); exit(1); } while(0)

static void curl_perform(CURL* curl) {
  int retries = 5;
  while(1) {
    CURLcode res = curl_easy_perform(curl);

    switch(res) {
      case CURLE_OK: 
        return;
      case CURLE_COULDNT_RESOLVE_PROXY:
      case CURLE_COULDNT_RESOLVE_HOST:
      case CURLE_COULDNT_CONNECT:
        fprintf(stderr, "curl: %s\n", curl_error_buf);
        if(retries-- <= 0) 
          exit(1);

        fprintf(stderr, "retrying...\n");
        sleep(3);
        break;
      default:
        fprintf(stderr, "curl: %s\n", curl_error_buf);
        exit(1);
    }
  }
}

char curl_error_buf[CURL_ERROR_SIZE];

/*
 * returns a prepared curl handle.
 *
 * Note that we always work with the same curl handle. This seems to work great,
 * and it means we don't have to clean up handles, because all we leak is just
 * that one handle, and it is going to be reused all the time anyways.
 *
 * THIS ALSO MEANS THAT THIS CODE IS NOT THREADSAFE, and propably NOT REENTRANT.
 */
#define MAX_HANDLES 10

static CURL* curl_handle(int index) {
  static int curl_initialised = 0;
  static CURL *curl_handles[MAX_HANDLES];

  if(!curl_initialised) {
    curl_initialised = 1;
    memset(curl_handles, 0, sizeof(curl_handles));
    curl_global_init(CURL_GLOBAL_ALL);  /* In windows, this will init the winsock stuff */ 
    atexit(curl_global_cleanup);
  }

  CURL* curl = curl_handles[index];
  if(!curl) {
    curl = curl_handles[index] = curl_easy_init();
    if(!curl)
      die("curl");
  }

  curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

  curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 1);
  curl_easy_setopt(curl, CURLOPT_USERAGENT, SSE_CLIENT_USERAGENT);

  curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10);
  curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, curl_error_buf);
  
  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, options.allow_insecure ? 0L : 1L);

  curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, options.allow_insecure ? 0L : 1L);

  if(options.ssl_cert)
    curl_easy_setopt(curl, CURLOPT_SSLCERT, options.ssl_cert);
  
  if(options.ca_info) 
    curl_easy_setopt(curl, CURLOPT_CAINFO, options.ca_info);
  
  return curl;
}

size_t http_ignore_data(char *ptr, size_t size, size_t nmemb, void *userdata)
{ 
  return size * nmemb; 
}

extern void http(int verb,
  const char*   url, 
  const char**  http_headers,   
  const char*   body, 
  unsigned      bodyLength,
  size_t        (*on_data)(char *ptr, size_t size, size_t nmemb, void *userdata),
  const char*   (*on_verify)(CURL* curl)
)
{
  /* init curl handle */
  CURL *curl = curl_handle(verb);

  curl_easy_setopt(curl, CURLOPT_URL, url);

  curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
  
  struct curl_slist *headers = NULL;
  while(http_headers && *http_headers)
    headers = curl_slist_append(headers, *http_headers++);

  if(verb == HTTP_POST) { // it will never be POST in our impl.
    /*
     * HTTP 1.1 specifies that a POST should use a "Expect: 100-continue" 
     * header. This should prepare the server for a potentially large 
     * upload.
     *
     * Our bodies are not that big anyways, and some servers - notably thin
     * running standalone does not send these - leading to a one or two-seconds
     * delay. 
     *
     * Therefore we disable this behaviour.
     */
    headers = curl_slist_append(headers, "Expect:");
  }
  
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers); 
  
 
  if(verb == HTTP_POST) {
    curl_easy_setopt(curl, CURLOPT_POST, 1L);                     /* set POST */ 
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);             /* set POST data */
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, bodyLength);
  }
  
 
  if(on_data)
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, on_data);

  curl_perform(curl);

  long response_code; 
  const char* effective_url = 0;
  
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code); 
  if(response_code < 200 || response_code >= 300) {
    if(!effective_url)
      curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effective_url); 
    
    fprintf(stderr, "%s: HTTP(S) status code %ld\n", effective_url, response_code);
    exit(1);
  }

  const char* verification_error = on_verify ? on_verify(curl) : 0;
  if(verification_error) {
    if(!effective_url)
      curl_easy_getinfo(curl, CURLINFO_EFFECTIVE_URL, &effective_url); 

    fprintf(stderr, "%s: %s\n", effective_url, verification_error);
    exit(1);
  }

  if(headers)
    curl_slist_free_all(headers);

  curl_easy_cleanup(curl);

}
