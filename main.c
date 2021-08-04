/*
  Copyright (C) 2017, 2021 Vasily Evseenko <svpcom@p2ptech.org>
  based on PlayuavOSD https://github.com/TobiasBales/PlayuavOSD.git
*/

/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */


#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <poll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "osdrender.h"
#include "osdmavlink.h"
#include "osdvar.h"
#include "osdconfig.h"
#include "UAVObj.h"
#include "graphengine.h"


int open_udp_socket_for_rx(int port)
{
    struct sockaddr_in saddr;
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0){
        perror("Error opening socket");
        exit(1);
    }

    printf("OSD_PORT=%d\n", port);

    int optval = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

    bzero((char *) &saddr, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);
    saddr.sin_port = htons((unsigned short)port);

    if (bind(fd, (struct sockaddr *) &saddr, sizeof(saddr)) < 0)
    {
        perror("Bind error");
        exit(1);
    }
    return fd;
}

int main(int argc, char **argv)
{
    uint64_t render_ts = 0;
    uint64_t cur_ts = 0;
    uint8_t buf[65536]; // Max UDP packet size
    int fd;
    struct pollfd fds[1];
    int shift_x = 0;
    int shift_y = 0;
    float scale_x = 1.0;
    float scale_y = 1.0;
    char *osd_port = getenv("OSD_PORT");

    osd_init(shift_x, shift_y, scale_x, scale_y);

#ifdef __GST_CAIRO__
    void* gst_thread_start(void *arg)
    {
        gst_main(argc, argv);
        return NULL;
    }

    pthread_t tid;
    pthread_create(&tid, NULL, gst_thread_start, NULL);
#endif

    memset(fds, '\0', sizeof(fds));
    fd = open_udp_socket_for_rx(osd_port == NULL ? 14551 : atoi(osd_port));

    if(fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK) < 0)
    {
        perror("Unable to set socket into nonblocked mode");
        exit(1);
    }

    fds[0].fd = fd;
    fds[0].events = POLLIN;

    while(1)
    {
        cur_ts = GetSystimeMS();
        uint64_t sleep_ts = render_ts > cur_ts ? render_ts - cur_ts : 0;
        int rc = poll(fds, 1, sleep_ts);

        if (rc < 0){
            if (errno == EINTR || errno == EAGAIN) continue;
            perror("Poll error");
            exit(1);
        }

        if (fds[0].revents & (POLLERR | POLLNVAL))
        {
            fprintf(stderr, "socket error!");
            exit(1);
        }

        if (fds[0].revents & POLLIN){
            ssize_t rsize;
            while((rsize = recv(fd, buf, sizeof(buf), 0)) >= 0)
            {
                parse_mavlink_packet(buf, rsize);
            }
            if (rsize < 0 && errno != EWOULDBLOCK){
                perror("Error receiving packet");
                exit(1);
            }
        }

        cur_ts = GetSystimeMS();
        if (render_ts <= cur_ts)
        {
            render_ts = cur_ts + 1000 / 10; // 10 Hz max
            render();
        }
    }

    return 0;
}
