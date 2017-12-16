#ifndef __OSDVAR_H
#define __OSDVAR_H

#include <math.h>
#include <stdbool.h>
#include <stdint.h>

/////////////////////////////////////////////////////////////////////////
extern uint64_t lastWritePanel;
extern uint8_t mav_type;
extern uint8_t mav_system;
extern uint8_t mav_component;
extern uint64_t sys_start_time;
extern uint64_t armed_start_time;
extern uint64_t total_armed_time;
extern uint8_t vtol_state;

/////////////////////////////////////////////////////////////////////////
extern float osd_vbat_A;                 // Battery A voltage in milivolt
extern int16_t osd_curr_A;                 // Battery A current
extern int8_t osd_battery_remaining_A;    // 0 to 100 <=> 0 to 1000
extern uint32_t osd_curr_consumed_mah; // total current drawn since startup in amp-hours

extern float osd_pitch;                  // pitch from DCM
extern float osd_roll;                   // roll from DCM
extern float osd_yaw;                    // relative heading form DCM
extern float osd_heading;                // ground course heading from GPS

extern float osd_lat;                    // latidude
extern float osd_lon;                    // longitude
extern uint8_t osd_satellites_visible;     // number of satelites
extern uint8_t osd_fix_type;               // GPS lock 0-1=no fix, 2=2D, 3=3D
extern double osd_hdop;

extern float osd_lat2;                    // latidude
extern float osd_lon2;                    // longitude
extern uint8_t osd_satellites_visible2;     // number of satelites
extern uint8_t osd_fix_type2;               // GPS lock 0-1=no fix, 2=2D, 3=3D
extern double osd_hdop2;

extern float osd_airspeed;               // airspeed
extern float osd_groundspeed;            // ground speed
extern float osd_downVelocity;           // ground speed
extern uint16_t osd_throttle;            // throtle
extern float osd_alt;                    // altitude
extern float osd_rel_alt;                // relative altitude	//  jmmods
extern float osd_bottom_clearance;       // relative altitude	//  jmmods
extern float osd_climb;
extern float osd_climb_ma[10];
extern int osd_climb_ma_index;
extern float osd_total_trip_dist; //total trip distance since startup, calculated in meter

extern float nav_roll; // Current desired roll in degrees
extern float nav_pitch; // Current desired pitch in degrees
extern int16_t nav_bearing; // Current desired heading in degrees
extern int16_t wp_target_bearing; // Bearing to current MISSION/target in degrees
extern int8_t wp_target_bearing_rotate_int;
extern uint16_t wp_dist; // Distance to active MISSION in meters
extern uint8_t wp_number; // Current waypoint number
extern float alt_error; // Current altitude error in meters
extern float aspd_error; // Current airspeed error in meters/second
extern float xtrack_error; // Current crosstrack error on x-y plane in meters
extern float eff; //Efficiency

extern uint32_t custom_mode;
extern bool motor_armed;
extern bool last_motor_armed;
extern uint8_t base_mode;
extern uint8_t autopilot;

extern bool osd_chan_cnt_above_eight;
extern uint16_t osd_chan1_raw;
extern uint16_t osd_chan2_raw;
extern uint16_t osd_chan3_raw;
extern uint16_t osd_chan4_raw;
extern uint16_t osd_chan5_raw;
extern uint16_t osd_chan6_raw;
extern uint16_t osd_chan7_raw;
extern uint16_t osd_chan8_raw;
extern uint16_t osd_chan9_raw;
extern uint16_t osd_chan10_raw;
extern uint16_t osd_chan11_raw;
extern uint16_t osd_chan12_raw;
extern uint16_t osd_chan13_raw;
extern uint16_t osd_chan14_raw;
extern uint16_t osd_chan15_raw;
extern uint16_t osd_chan16_raw;
extern uint8_t osd_rssi; //raw value from mavlink

extern uint8_t osd_got_home;               // tels if got home position or not
extern float osd_home_lat;               // home latidude
extern float osd_home_lon;               // home longitude
extern float osd_home_alt;
extern long osd_home_distance;          // distance from home
extern uint32_t osd_home_bearing;
extern uint8_t osd_alt_cnt;              // counter for stable osd_alt
extern float osd_alt_prev;             // previous altitude

extern float osd_windSpeed;
extern float osd_windDir;

extern volatile uint8_t current_panel;

extern float atti_mp_scale;
extern float atti_3d_scale;
extern uint32_t atti_3d_min_clipX;
extern uint32_t atti_3d_max_clipX;
extern uint32_t atti_3d_min_clipY;
extern uint32_t atti_3d_max_clipY;

#define MAX_WAYPOINTS   20

extern uint8_t got_mission_counts;
extern uint8_t enable_mission_count_request;
extern uint16_t mission_counts;
extern uint8_t enable_mission_item_request;
extern uint16_t current_mission_item_req_index;

extern uint16_t wp_counts;
extern uint8_t got_all_wps;
// a self contained waypoint list
typedef struct WAYPOINT_TYP {
//	float para1;
//    float para2;
//    float para3;
//    float para4;

  float x;
  float y;
  float z;

  uint16_t seq;
  uint16_t cmd;

//    uint8_t frame;
  uint8_t current;
//    uint8_t autocontinue;
} WAYPOINT, *WAYPOINT_PTR;

extern WAYPOINT wp_list[MAX_WAYPOINTS];

extern int8_t osd_offset_Y;
extern int8_t osd_offset_X;
#endif
