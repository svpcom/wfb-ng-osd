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
/*
 * With Grateful Acknowledgements to the projects:
 * MinimOSD - arducam-osd Controller(https://code.google.com/p/arducam-osd/)
 */
#include "osdvar.h"

/////////////////////////////////////////////////////////////////////////
uint8_t mavbeat = 0;
uint64_t lastMAVBeat = 0;
uint64_t lastWritePanel = 0;
uint8_t waitingMAVBeats = 1;
uint8_t mav_type;
uint8_t mav_system;
uint8_t mav_component;
uint8_t enable_mav_request = 0;
uint64_t sys_start_time = 0;
uint64_t heatbeat_start_time = 0;
uint64_t armed_start_time = 0;
uint64_t total_armed_time = 0;

/////////////////////////////////////////////////////////////////////////
float osd_vbat_A = 0.0f;                 // Battery A voltage in milivolt
int16_t osd_curr_A = 0;                 // Battery A current
int8_t osd_battery_remaining_A = 0;    // 0 to 100 <=> 0 to 1000
uint32_t osd_curr_consumed_mah = 0;

float osd_pitch = 0.0f;                  // pitch from DCM
float osd_roll = 0.0f;                   // roll from DCM
float osd_yaw = 0.0f;                    // relative heading form DCM
float osd_heading = 0.0f;                // ground course heading from GPS

float osd_lat = 0.0f;                    // latidude
float osd_lon = 0.0f;                    // longitude
uint8_t osd_satellites_visible = 0;     // number of satelites
uint8_t osd_fix_type = 0;               // GPS lock 0-1=no fix, 2=2D, 3=3D
double osd_hdop = 0.0f;

float osd_lat2 = 0.0f;                    // latidude
float osd_lon2 = 0.0f;                    // longitude
uint8_t osd_satellites_visible2 = 0;     // number of satelites
uint8_t osd_fix_type2 = 0;               // GPS lock 0-1=no fix, 2=2D, 3=3D
double osd_hdop2 = 0.0f;

float osd_airspeed = -1.0f;              // airspeed
float osd_groundspeed = 0.0f;            // ground speed
float osd_downVelocity = 0.0f;
uint16_t osd_throttle = 0;               // throtle
float osd_alt = 0.0f;                    // altitude
float osd_rel_alt = 0.0f;                                // relative altitude	//  jmmods
float osd_bottom_clearance = NAN;
float osd_climb = 0.0f;
float osd_climb_ma[10];
int osd_climb_ma_index = 0;
float osd_total_trip_dist = 0;

float nav_roll = 0.0f; // Current desired roll in degrees
float nav_pitch = 0.0f; // Current desired pitch in degrees
int16_t nav_bearing = 0; // Current desired heading in degrees
int16_t wp_target_bearing = 0; // Bearing to current MISSION/target in degrees
int8_t wp_target_bearing_rotate_int = 0;
uint16_t wp_dist = 0; // Distance to active MISSION in meters
uint8_t wp_number = 0; // Current waypoint number
float alt_error = 0.0f; // Current altitude error in meters
float aspd_error = 0.0f; // Current airspeed error in meters/second
float xtrack_error = 0.0f; // Current crosstrack error on x-y plane in meters
float eff = 0.0f; //Efficiency
uint8_t osd_linkquality = 0;

bool motor_armed = false;
bool last_motor_armed = false;
uint8_t autopilot = 0;
uint8_t base_mode = 0;
uint32_t custom_mode = 0;

bool osd_chan_cnt_above_eight = false;
uint16_t osd_chan1_raw = 0;
uint16_t osd_chan2_raw = 0;
uint16_t osd_chan3_raw = 0;
uint16_t osd_chan4_raw = 0;
uint16_t osd_chan5_raw = 0;
uint16_t osd_chan6_raw = 0;
uint16_t osd_chan7_raw = 0;
uint16_t osd_chan8_raw = 0;
uint16_t osd_chan9_raw = 0;
uint16_t osd_chan10_raw = 0;
uint16_t osd_chan11_raw = 0;
uint16_t osd_chan12_raw = 0;
uint16_t osd_chan13_raw = 0;
uint16_t osd_chan14_raw = 0;
uint16_t osd_chan15_raw = 0;
uint16_t osd_chan16_raw = 0;
uint8_t osd_rssi = 0; //raw value from mavlink

uint8_t osd_got_home = 0;               // tels if got home position or not
float osd_home_lat = 0.0f;               // home latidude
float osd_home_lon = 0.0f;               // home longitude
float osd_home_alt = 0.0f;
long osd_home_distance = 0;          // distance from home
uint32_t osd_home_bearing = 0;
uint8_t osd_alt_cnt = 0;              // counter for stable osd_alt
float osd_alt_prev = 0.0f;             // previous altitude

float osd_windSpeed = 0.0f;
float osd_windDir = 0.0f;

volatile uint8_t current_panel = 1;

float atti_mp_scale = 0.0;
float atti_3d_scale = 0.0;
uint32_t atti_3d_min_clipX = 0;
uint32_t atti_3d_max_clipX = 0;
uint32_t atti_3d_min_clipY = 0;
uint32_t atti_3d_max_clipY = 0;

uint8_t got_mission_counts = 0;
uint8_t enable_mission_count_request = 0;
uint16_t mission_counts = 0;
uint8_t enable_mission_item_request = 0;
uint16_t current_mission_item_req_index = 0;

uint16_t wp_counts = 0;
uint8_t got_all_wps = 0;
WAYPOINT wp_list[MAX_WAYPOINTS];

int8_t osd_offset_Y = 0;
int8_t osd_offset_X = 0;
