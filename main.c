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


#ifdef __GST_OPENGL__
int gst_main(int rtp_port, char *codec, int rtp_jitter, osd_render_t osd_render, int screen_width, char *rtsp_url);
#endif

static volatile uint8_t finished = 0;
int osd_debug = 0;

void sigterm_handler(int signum)
{
    finished = 1;
}

int open_udp_socket_for_rx(int port)
{
    struct sockaddr_in saddr;
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0){
        perror("Error opening socket");
        exit(1);
    }

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
    int opt;
    int osd_port = 14551;
    int rtp_port = 5600;
    char* codec = "h264";
    int rtp_jitter = 5;
    osd_render_t osd_render = OSD_RENDER_GL;
    int screen_width = 1920;
    char *rtsp_url = NULL;

    uint64_t render_ts = 0;
    uint64_t cur_ts = 0;
    uint8_t buf[65536]; // Max UDP packet size
    int fd;
    struct pollfd fds[1];

    while ((opt = getopt(argc, argv, "hdp:P:R:45j:xaw:")) != -1) {
        switch (opt) {
        case 'p':
            osd_port = atoi(optarg);
            break;

        case 'P':
            rtp_port = atoi(optarg);
            break;

        case 'R':
            rtsp_url = strdup(optarg);
            break;

        case '4':
            codec = "h264";
            break;

        case '5':
            codec = "h265";
            break;

        case 'j':
            rtp_jitter = atoi(optarg);
            break;

        case 'x':
            osd_render = OSD_RENDER_XV;
            break;

        case 'a':
            osd_render = OSD_RENDER_AUTO;
            break;

        case 'w':
            screen_width = atoi(optarg);
            break;

        case 'd':
            osd_debug = 1;
            break;

        case 'h':
        default:
        show_usage:

#ifdef __GST_OPENGL__
            fprintf(stderr, "%s [-p mavlink_port] [-P rtp_port] [ -R rtsp_url ] [-4] [-5] [-j rtp_jitter] [-x] [-a] [-w screen_width] \n", argv[0]);
            fprintf(stderr, "Default: mavlink_port=%d, rtp_port=%d, rtsp_url=%s, codec=%s, rtp_jitter=%d, screen_width=%d\n",
                    osd_port, rtp_port,
                    rtsp_url != NULL ? rtsp_url : "none",
                    codec, rtp_jitter, screen_width);
#else
            fprintf(stderr, "%s [-p mavlink_port]\n", argv[0]);
            fprintf(stderr, "Default: mavlink_port=%d\n", osd_port);
#endif
            fprintf(stderr, "WFB-ng OSD version " WFB_OSD_VERSION "\n");
            fprintf(stderr, "WFB-ng home page: <http://wfb-ng.org>\n");
            exit(1);
        }
    }

    if (optind > argc) {
        goto show_usage;
    }

#ifdef __GST_OPENGL__
    printf("Use: mavlink_port=%d, rtp_port=%d, rtsp_url=%s, codec=%s, rtp_jitter=%d, osd_render=%d, screen_width=%d\n",
           osd_port, rtp_port,
           rtsp_url != NULL ? rtsp_url : "none",
           codec, rtp_jitter, osd_render, screen_width);

    osd_init(0, 0, 1, 1);
    fd = open_udp_socket_for_rx(osd_port);

    void* gst_thread_start(void *arg)
    {
        gst_main(rtp_port, codec, rtp_jitter, osd_render, screen_width, rtsp_url);
        fprintf(stderr, "gst thread exited\n");
        exit(1);
    }

    pthread_t tid;
    pthread_create(&tid, NULL, gst_thread_start, NULL);

    while(1)
    {
        ssize_t rsize;
        while((rsize = recv(fd, buf, sizeof(buf), 0)) >= 0)
        {
            // Avoid race with rendering in gstreamer
            pthread_mutex_lock(&video_mutex);
            parse_mavlink_packet(buf, rsize);
            pthread_mutex_unlock(&video_mutex);
        }

        if (rsize < 0 && errno != EINTR)
        {
            perror("Error receiving packet");
            exit(1);
        }
    }

#else
    printf("Use mavlink_port=%d\n", osd_port);

    osd_init(0, 0, 1, 1);
    fd = open_udp_socket_for_rx(osd_port);

    if(fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK) < 0)
    {
        perror("Unable to set socket into nonblocked mode");
        exit(1);
    }

    memset(fds, '\0', sizeof(fds));
    fds[0].fd = fd;
    fds[0].events = POLLIN;

    signal(SIGTERM, sigterm_handler);
    signal(SIGINT, sigterm_handler);

    fprintf(stderr, "Starting event loop\n");
    while(!finished)
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
            render_ts = cur_ts + 1000 / 30; // 30Hz osd refresh rate
            render();
        }
    }
    fprintf(stderr, "Event loop finished\n");
#endif
    return 0;
}
