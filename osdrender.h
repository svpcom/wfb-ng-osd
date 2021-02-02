#ifndef __RENDER_H__
#define __RENDER_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include <string.h>
#include <utime.h>
#include <unistd.h>
#include <endian.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <time.h>

#include "graphengine.h"
#include "mavlink/common/mavlink.h"

void osd_init(int shift_x, int shift_y, float scale_x, float scale_y);
void render();


/// GPS status codes
enum GPS_Status {
  NO_GPS = 0,                   ///< No GPS connected/detected
  NO_FIX = 1,                   ///< Receiving valid GPS messages but no lock
  GPS_OK_FIX_2D = 2,            ///< Receiving valid messages and 2D lock
  GPS_OK_FIX_3D = 3,            ///< Receiving valid messages and 3D lock
  GPS_OK_FIX_3D_DGPS = 4,       ///< Receiving valid messages and 3D lock with differential improvements
  GPS_OK_FIX_3D_RTK = 5,        ///< Receiving valid messages and 3D lock, with relative-positioning improvements
};


uint64_t GetSystimeMS(void);
void RenderScreen(void);

void draw_uav3d(void);
void draw_uav2d(void);
void draw_throttle(void);
void draw_simple_attitude(void);
void draw_home_direction(void);
void draw_radar(void);
void draw_flight_mode(void);
void draw_arm_state(void);
void draw_battery_voltage(void);
void draw_battery_current(void);
void draw_battery_remaining(void);
void draw_battery_consumed(void);
void draw_altitude_scale(void);
void draw_absolute_altitude(void);
void draw_relative_altitude(void);
void draw_speed_scale(void);
void draw_ground_speed(void);
void draw_air_speed(void);
void draw_vtol_speed(void);
void draw_home_latitude(void);
void draw_home_longitude(void);
void draw_gps_status(void);
void draw_gps_hdop(void);
void draw_gps_latitude(void);
void draw_gps_longitude(void);
void draw_gps2_status(void);
void draw_gps2_hdop(void);
void draw_gps2_latitude(void);
void draw_gps2_longitude(void);
void draw_total_trip(void);
void draw_time(void);
void draw_climb_rate(void);
void draw_rssi(void);
void draw_wfb_state(void);
void draw_link_quality(void);
void draw_efficiency(void);
void draw_wind(void);
void draw_map(void);
void draw_panel_changed(void);
void draw_warning(void);
void draw_CWH(void);
void draw_head_wp_home(void);
void draw_osd_messages(void);

void cal_vars(void);


void draw_vertical_scale(float v, int range, int halign, int x, int y, int height, int mintick_step,
                         int majtick_step, int mintick_len, int majtick_len,
                         int boundtick_len, __attribute__((unused)) int max_val, int flags);

void draw_linear_compass(int v, int home_dir, int range, int width, int x, int y, int mintick_step,
                         int majtick_step, int mintick_len, int majtick_len,
                         __attribute__((unused)) int flags);



#endif
