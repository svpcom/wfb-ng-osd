#ifndef __OSD_MAVLINK_H
#define __OSD_MAVLINK_H

#include "mavlink/common/mavlink.h"

void parse_mavlink_packet(uint8_t *buf, int buflen);

#endif  //__OSD_MAVLINK_H
