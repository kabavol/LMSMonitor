#include "cnnxn.h"
#include "common.h"
#include "display.h"
#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void showConnect(void) {

    char stbl[BSIZE];
    char hostbuffer[256];
    struct hostent *host_entry;
    int hostname;

    hostname = gethostname(hostbuffer, sizeof(hostbuffer));

    int xpos = 10;
    int ypos = 4;
    putTextToCenter(ypos, hostbuffer);

    sprintf(stbl, "%s %s\n", labelIt("Hostname", LABEL_WIDTH, "."), hostbuffer);
    putMSG(stbl, LL_QUIET);

    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return;
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host,
                        NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

        if ((!strcmp(ifa->ifa_name, "lo") == 0) &&
            (ifa->ifa_addr->sa_family == AF_INET)) {
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                return;
            }
            sprintf(stbl, "%s %s\n", labelIt("Interface", LABEL_WIDTH, "."),
                    ifa->ifa_name);
            putMSG(stbl, LL_QUIET);
            sprintf(stbl, "%s %s\n", labelIt("Address", LABEL_WIDTH, "."),
                    host);
            putMSG(stbl, LL_QUIET);
            int icon = ((strstr(ifa->ifa_name, "wlan") == NULL) ? 1 : 0);
            ypos += 16;
            putIFDetail(icon, xpos, ypos, host);
        }
    }

    freeifaddrs(ifaddr);
    refreshDisplay();
    dodelay(1500);
    dodelay(1500);
    dodelay(1500);
}
