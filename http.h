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

#ifndef VISSY_HTTP_H
#define VISSY_HTTP_H

#include <string.h>
#include <curl/curl.h>

#define HTTP_GET  0
#define HTTP_POST 1

extern void http(int  verb,
  const char*   url, 
  const char**  http_headers, 
  const char*   body, 
  unsigned      bodyLength,
  size_t        (*on_data)(char *ptr, size_t size, size_t nmemb, void *userdata),
  const char*   (*on_verify)(CURL* curl)
);

extern char curl_error_buf[];
extern size_t http_ignore_data(char *ptr, size_t size, size_t nmemb, void *userdata);

#endif