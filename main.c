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


int main(int argc, char *argv[]) {
    uint64_t render_ts = 0;
    uint64_t cur_ts = 0;
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    int fd;
    struct pollfd fds[1];

    osd_init();

    memset(fds, '\0', sizeof(fds));
    fd = open_udp_socket_for_rx(14550);

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
        //printf("SLEEP %Lu\n", sleep_ts);
        int rc = poll(fds, 1, sleep_ts); // 30 hz

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
        //printf("R TS %Lu C TS %Lu\n", render_ts, cur_ts);
        if (render_ts <= cur_ts)
        {
            //printf(".");
            //fflush(stdout);
            render_ts = cur_ts + 1000 / 10; // 10 Hz max
            render();
        }
    }

    return 0;
}
