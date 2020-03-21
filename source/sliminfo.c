/*
 *	sliminfo.c
 *
 *	(c) 2015 László TÓTH
 *	(c) 2020 Stuart Hunter
 *
 *	TODO:	Reconnect to server
 *	TODO:	Clock when not playing
 *	TODO:	Visualization (shmem or SSE stream)
 *	TODO:	Weather service
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
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "common.h"
#include "sliminfo.h"
#include "tagUtils.h"

int LMSPort;
char *LMSHost = NULL;

char playerIP[BSIZE] = {0};
char playerID[BSIZE] = {0};
char query[BSIZE] = {0};
char stb[BSIZE];

int sockFD = 0;
struct sockaddr_in serv_addr;

tag tagStore[MAXTAG_TYPES];
bool refreshRequ;
pthread_t sliminfoThread;

char *getPlayerIP(void){
    return (char *)playerIP;
}

int discoverPlayer(char *playerName) {
    char qBuffer[BSIZE];
    char aBuffer[BSIZE];
    int bytes;

    // I've not found this feature in the CLI spec,
    //	but if you send the player name, the server
    //  answers with the unique player ID (usually MAC address).
    if (playerName != NULL) {
        if (strlen(playerName) > (BSIZE / 3)) {
            abortMonitor("ERROR too long player name!");
        }

        // submitting the player no longer comes back with the correct ID and IP
        // timing is such that this coincided with the 6.0.0 upgrade
        // retool: query players long form and loop over to find the one we want
        sprintf(qBuffer, "players 0 99\n");
        if (write(sockFD, qBuffer, strlen(qBuffer)) < 0) {
            abortMonitor("ERROR writing to socket!");
        }
        if ((bytes = read(sockFD, aBuffer, BSIZE - 1)) < 0) {
            abortMonitor("ERROR reading from socket!");
        }
        aBuffer[bytes] = 0;

        int playerCount = -1;
        sscanf(aBuffer, "%*[^A]A%d[^ ]", &playerCount);
        if (playerCount>0) // we haveplayers, find our guy
        {
            if (strncmp(aBuffer, playerName, strlen(playerName)) == 0) {
                sprintf(aBuffer, "Player specified \"%s\" not found, supply corrected name!", playerName);
                abortMonitor(aBuffer);
            }
            printf("Player Count ...: %d\n", playerCount);
            // if playercount > 1 tokenize else just decompose
            char pind[15] = "playerindex%3A";
            multi_tok_t mt = multi_tok_init();
            char *player = multi_tok(aBuffer, &mt, pind);
            while (player != NULL) {
                if (strstr(player, playerName) != 0) {
                    if (getTag("playerid", player, aBuffer, MAXTAG_DATA))
                        strcpy(playerID, aBuffer);
                    if (getTag("ip", player, aBuffer, MAXTAG_DATA))
                    {
                        int tl;
                        char *pport;
                        if ((pport = strstr(aBuffer, ":")) != NULL) {
                            tl = pport - aBuffer;
                        } else {
                            tl = strlen(aBuffer);
                        }
                        strncpy(playerIP, aBuffer, tl);
                    }
                    break;
                }
                player = multi_tok(NULL, &mt, pind);
            }

        }

    } else {
        return -1;
    }

    sprintf(stb, "Player Name ....: %s\nPlayer ID ......: %s\nPlayer IP ......: %s\n",
            playerName, playerID, playerIP);
    putMSG(stb, LL_INFO);

    return 0;
}

int setStaticServer(void) {
    LMSPort = 9090;
    LMSHost = NULL; // autodiscovery
    return 0;
}

/*
 * LMS server discover
 *
 * code from: https://code.google.com/p/squeezelite/source/browse/slimproto.c
 *
 */
in_addr_t getServerAddress(void) {
#define PORT 3483

    struct sockaddr_in d;
    struct sockaddr_in s;
    char *buf;
    struct pollfd pollinfo;

    if (LMSHost != NULL) {
        // Static server address
        memset(&s, 0, sizeof(s));
        inet_pton(AF_INET, LMSHost, &(s.sin_addr));

    } else {
        // Discover server address
        int disc_sock = socket(AF_INET, SOCK_DGRAM, 0);

        socklen_t enable = 1;
        setsockopt(disc_sock, SOL_SOCKET, SO_BROADCAST, (const void *)&enable,
                   sizeof(enable));

        buf = (char *)"e";

        memset(&d, 0, sizeof(d));
        d.sin_family = AF_INET;
        d.sin_port = htons(PORT);
        d.sin_addr.s_addr = htonl(INADDR_BROADCAST);

        pollinfo.fd = disc_sock;
        pollinfo.events = POLLIN;

        do {
            putMSG("Sending LMS Discovery ...\n", LL_INFO);
            memset(&s, 0, sizeof(s));

            if (sendto(disc_sock, buf, 1, 0, (struct sockaddr *)&d, sizeof(d)) <
                0) {
                putMSG("Error sending disovery\n", LL_INFO);
            }

            if (poll(&pollinfo, 1, 5000) == 1) {
                char readbuf[10];
                socklen_t slen = sizeof(s);
                recvfrom(disc_sock, readbuf, 10, 0, (struct sockaddr *)&s,
                         &slen);
                sprintf(stb, "LMS (Server) responded:\nServer IP ....: %s:%d\n",
                        inet_ntoa(s.sin_addr), ntohs(s.sin_port));
                putMSG(stb, LL_INFO);
            }
        } while (s.sin_addr.s_addr == 0);

        close(disc_sock);
    }

    return s.sin_addr.s_addr;
}

char *player_mac(void) { return playerID; }

void askRefresh(void) { refreshRequ = true; }

bool isRefreshed(void) { return !refreshRequ; }

void refreshed(void) { refreshRequ = false; }

int connectServer(void) {
    int sfd;

    if (setStaticServer() < 0) {
        abortMonitor("Failed to find LMS server!");
    }

    if ((sfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        abortMonitor("Failed to opening socket!");
    }

    memset(&serv_addr, 0, sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = getServerAddress();
    serv_addr.sin_port = htons(LMSPort);

    if (connect(sfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        fprintf(stderr, "No such host: %s\n", inet_ntoa(serv_addr.sin_addr));
        close(sfd);
        return -1;
    }

    return sfd;
}

void closeSliminfo(void) {
    if (sockFD > 0) {
        close(sockFD);
    }
    for (int i = 0; i < MAXTAG_TYPES; i++) {
        if (tagStore[i].tagData != NULL) {
            free(tagStore[i].tagData);
        }
    }
    pthread_cancel(sliminfoThread);
    pthread_join(sliminfoThread, NULL);
}

tag *initTagStore(void) {

    tagStore[SAMPLESIZE].name = "samplesize";
    tagStore[SAMPLERATE].name = "samplerate";
    tagStore[TIME].name = "time";
    tagStore[DURATION].name = "duration";
    tagStore[REMAINING].name = "remaining";
    tagStore[VOLUME].name = "mixer%20volume";
    tagStore[TITLE].name = "title";
    tagStore[ALBUM].name = "album";
    tagStore[YEAR].name = "year";
    tagStore[ARTIST].name = "artist";
    tagStore[ALBUMARTIST].name = "albumartist";
    tagStore[COMPOSER].name = "composer";
    tagStore[CONDUCTOR].name = "conductor";
    tagStore[MODE].name = "mode";
    tagStore[COMPILATION].name = "compilation";

    for (int i = 0; i < MAXTAG_TYPES; i++) {
        if ((tagStore[i].tagData =
                 (char *)malloc(MAXTAG_DATA * sizeof(char))) == NULL) {
            closeSliminfo();
            return NULL;
        } else {
            strcpy(tagStore[i].tagData, "");
            tagStore[i].displayName = "";
            tagStore[i].valid = false;
            tagStore[i].changed = false;
        }
    }
    return tagStore;
}

void *serverPolling(void *x_voidptr) {

    char variousArtist[BSIZE] = "Various Artists";
    char buffer[BSIZE];
    char dbuffer[BSIZE];
    char tagData[BSIZE];
    int rbytes;

int z = 0;

    while (true) {

        if (!isRefreshed()) {

            if (write(sockFD, query, strlen(query)) < 0) {
                abortMonitor("ERROR writing to socket");
            }
            if ((rbytes = read(sockFD, buffer, BSIZE - 1)) < 0) {
                abortMonitor("ERROR reading from socket");
            }
            buffer[rbytes] = 0;
/*
// works well but incurs good chunk of re-write
//(?P<name>[^:]*):?( ?(?P<value>.*))?
if (z < 20) {
    urldecode2(buffer, dbuffer);
    strncpy(dbuffer, replaceStr(
        dbuffer, replaceStr(query, "\n", " "), ""), BSIZE);
    printf("%s\n%s\n%s\n",
    buffer, 
    query, 
    dbuffer);
z++;
}
*/
            for (int i = 0; i < MAXTAG_TYPES; i++) {

                if ((getTag((char *)tagStore[i].name, buffer, tagData,
                            BSIZE)) != NULL) {
                    if ((i != REMAINING) &&
                        (strcmp(tagData, tagStore[i].tagData) != 0)) {
                        strncpy(tagStore[i].tagData, tagData, MAXTAG_DATA);
                        tagStore[i].changed = true;
                    }
                    tagStore[i].valid = true;
                } else {
                    tagStore[i].valid = false;
                }

                if (0 == 1) {
                    if (ALBUM == i) {
                        printf("< %d) %s: %s %d %d\n", i,
                               tagStore[i].displayName, tagStore[i].tagData,
                               tagStore[i].valid, tagStore[i].changed);
                    }
                }
            }

            long pTime = getMinute(&tagStore[TIME]);
            long dTime = getMinute(&tagStore[DURATION]);

            if (pTime && dTime) {
                snprintf(tagData, MAXTAG_DATA, "%ld", (dTime - pTime));
                if (strcmp(tagData, tagStore[REMAINING].tagData) != 0) {
                    strncpy(tagStore[REMAINING].tagData, tagData, MAXTAG_DATA);
                    tagStore[REMAINING].valid = true;
                    tagStore[REMAINING].changed = true;
                }
            }

            // Fix Various Artists
            if (getTagBool(&tagStore[COMPILATION])) {
                if (strcmp(variousArtist, tagStore[ALBUMARTIST].tagData) != 0) {
                    strncpy(tagStore[ALBUMARTIST].tagData, variousArtist,
                            MAXTAG_DATA);
                    tagStore[ALBUMARTIST].valid = true;
                    tagStore[ALBUMARTIST].changed = true;
                }
            }

            if (isEmptyStr(tagStore[ALBUMARTIST].tagData)) {
                if (isEmptyStr(tagStore[CONDUCTOR]
                                   .tagData)) { // override for conductor
                    strncpy(tagStore[ALBUMARTIST].tagData,
                            tagStore[ARTIST].tagData, MAXTAG_DATA);
                    tagStore[ALBUMARTIST].valid = true;
                    tagStore[ALBUMARTIST].changed = true;
                }
            } else if (strcmp(variousArtist, tagStore[ALBUMARTIST].tagData) ==
                       0) {
                tagStore[ALBUMARTIST].valid = true;
            }

            refreshed();
            sleep(1);

        } // !isRefreshed

    } // while

    return NULL;
}

tag *initSliminfo(char *playerName) {
    if (setStaticServer() < 0) {
        return NULL;
    }
    if ((sockFD = connectServer()) < 0) {
        return NULL;
    }
    if (discoverPlayer(playerName) < 0) {
        return NULL;
    }

    /*
    ARTIST = "a"
    ARTIST_ROLE = "A"
    BUTTONS = "B"
    COVERID = "c"
    COMPILATION = "C"
    DURATION = "d"
    ALBUM_ID = "e"
    FILESIZE = "f"
    GENRE = "g"
    GENRE_LIST = "G"
    DISC = "i"
    SAMPLESIZE = "I"
    COVERART = "j"
    ARTWORK_TRACK_ID = "J"
    COMMENT = "k"
    ARTWORK_URL = "K"
    ALBUM = "l"
    INFO_LINK = "L"
    BPM = "m"
    MUSICMAGIC_MIXABLE = "M"
    MODIFICATION_TIME = "n"
    REMOTE_TITLE = "N"
    CONTENT_TYPE = "o"
    GENRE_ID = "p"
    GENRE_ID_LIST = "P"
    DISC_COUNT = "q"
    BITRATE = "r"
    RATING = "R"
    ARTIST_ID = "s"
    ARTIST_ROLE_IDS = "S"
    TRACK_NUMBER = "t"
    SAMPLERATE = "T"
    URL = "u"
    TAG_VERSION = "v"
    LYRICS = "w"
    REMOTE = "x"
    ALBUM_REPLAY_GAIN = "X"
    YEAR = "y"
    REPLAY_GAIN = "Y"


*/

    sprintf(query, "%s status - 1 tags:aAlCITytrdx\n", playerID);
    int x = 0;

    if (initTagStore() != NULL) {
        askRefresh();
        if (pthread_create(&sliminfoThread, NULL, serverPolling, &x) != 0) {
            closeSliminfo();
            abortMonitor("Failed to create sliminfo thread!");
        }
    } else {
        return NULL;
    }

    return tagStore;
}

void sliminfoFinalize(void) {
    closeSliminfo();
}