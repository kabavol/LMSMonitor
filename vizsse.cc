#include "thinhttplib.h"
#include <cstdio>
#include <chrono>
#include <pthread.h>

#include "jsmn.h"
#include "log.h"

#include "visualize.h"
#include "common.h"
#include "vizsse.h"

using namespace httplib;

pthread_t vizSSEThread;
bool reconnect = true;
unsigned long retrylong = 5;    // minutes
unsigned long retrybase = 3000; // milliseconds
unsigned long retry = retrybase;
Options options;

bool on_sse_callback(const char *data, uint64_t len){
  std::string msg(data, len);
  if(msg.rfind("retry: ", 8) == 0){
    retry = std::stoul(msg.substr(7));
  }
  else
    parse_payload((char*)data, len);

  return true;

}

void on_sse_event(const char *data) {

    // parse mesage payload - it is simple JSON - keep it light-weight
    //
    struct vissy_meter_t vissy_meter = {
        .meter_type = {0},
        .channel_name = {"L", "R"},
        .is_mono = 0,
        .sample_accum = {0, 0},
        .floor = -96,
        .reference = 32768,
        .dBfs = {-1000, -1000},
        .dB = {-1000, -1000},
        .linear = {0, 0},
        .rms_bar = {0, 0},
        .channel_width = {192, 192},
        .bar_size = {6, 6},
        .channel_flipped = {0, 0},
        .clip_subbands = {0, 0},
        .numFFT = {9, 9},
        .sample_bin_chan = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
                            {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}},
    };

    parse_visualize((char *)data, &vissy_meter);
    visualize(&vissy_meter); // go visualize (if screen active)
}


void urlparse(const char *uri, char *host, uint *port, char *endpoint)
{
    sscanf(uri, "http://%99[^:]:%99d/%99[^\n]", host, port, endpoint);
}

static void parseSSEArguments(struct Options opt) {

    // set default url and allow_insecure is always set to true for now
    options.allow_insecure = true;

    if (opt.uri)
        options.uri = opt.uri;
    if (opt.host)
        options.host = opt.host;
    if (opt.port)
        options.port = opt.port;
    if (opt.endpoint)
        options.endpoint = opt.endpoint;

    if ((!options.host)&&(options.uri))
        urlparse((char*)options.uri, (char*)options.host, (uint*)&options.port, (char*)options.endpoint);

    if (!options.allow_insecure) {
        if (strncmp(options.uri, "https:", 6)) {
            fprintf(
                stderr,
                "Insecure connections not allowed, use -i, if necessary.\n");
            exit(1);
        }
    }

    char info[BSIZE];
    sprintf(info, "SSE Host .....: %s\nComms Port ...: %d\nEndpoint .....: %s\n",
            options.host, options.port, options.endpoint);
    printf("%s",info);

}

void *vizSSEPolling(void *x_voidptr) {

  Client cli( options.host, options.port );

  char agent[BSIZE];
  sprintf(agent,"%s/%s", APPNAME, VERSION);
  cli.set_follow_location(true);
  const httplib::Headers headers = {
    { "User-Agent", agent },
    { "Accept", "text/event-stream" },
    { "Cache-Control", "no-cache" },
    { "Connection", "Keep-Alive" }, // fixed case httplib not case tolerant!!!
  };

  while(reconnect){

    /////printf("connect...\n");
    //auto res = cli.Get("/visionon?subscribe=SA-VU", headers, on_sse_callback);
    auto res = cli.Get(options.endpoint, headers, on_sse_callback);

    if(res){
      //check that response code is 2xx
      reconnect = (res->status / 100) == 2;
      if (reconnect)
        std::this_thread::sleep_for(std::chrono::minutes(retrylong));
    }
    else{
      std::this_thread::sleep_for(std::chrono::milliseconds(retry));
      retry = retrybase;
    }

  }

  return 0;

}

bool setupSSE(struct Options opt) {

    // setup for REST call/connection
    parseSSEArguments(opt);

    if(!options.host)
        return false;

    printf("Initializing SSE ...\n");
    if (pthread_create(&vizSSEThread, NULL, vizSSEPolling, NULL) != 0) {
        vissySSEFinalize();
        toLog(1, "Failed to create SSE Visualization thread!");
        return false;
    }
    else
        printf("SSE active on %s:%d\n", options.host, options.port);

    return true;

}

void vissySSEFinalize(void) {
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

void parse_visualize(char *data, struct vissy_meter_t *vissy_meter) {

    int i;
    int r;
    jsmn_parser p;
    jsmntok_t t[128]; /* We expect no more than 128 tokens */

    uint8_t channel = 0;
    bool is_vu = false;
    char test[1024];

    jsmn_init(&p);
    r = jsmn_parse(&p, data, strlen(data), t, sizeof(t) / sizeof(t[0]));
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
            is_vu = (strncmp(test, "VU", 2) == 0);
            strncpy(vissy_meter->meter_type, test, 2);
            i++;
        } else if (jsoneq(data, &t[i], "channel") == 0) {
            /* We may additionally check if the value is either "true" or "false" */
            // ignore - we'll digest
            i++;
        } else if (jsoneq(data, &t[i], "name") == 0) {
            channel = (strncmp(test, "L", 1) == 0) ? 0 : 1;
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
            vissy_meter->numFFT[channel] = atoi(test);
            i++;
        } else if (jsoneq(data, &t[i], "FFT") == 0) {
            int j;
            //if ((is_vu) || (t[i + 1].type != JSMN_ARRAY)) {
            if ((is_vu) || (t[i + 1].type != JSMN_ARRAY)) {
                continue; // We expect FFT values to be an array of int
            }
            for (j = 0; j < t[i + 1].size; j++) {
                jsmntok_t *g = &t[i + j + 2];
                sprintf(test, "%.*s", g->end - g->start, data + g->start);
                vissy_meter->sample_bin_chan[channel][j] = atoi(test);
            }
            i += t[i + 1].size + 1;
        } else {
            continue; // unknown or we need to map
        }
    }
}
