/* -*- c -*-
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
/*
 * With Grateful Acknowledgements to the projects:
 * Tau Labs - Brain FPV Flight Controller(https://github.com/BrainFPV/TauLabs)
 */

#include <assert.h>
#include <sys/time.h>
#include <time.h>
#include "osdrender.h"
#include "graphengine.h"
#include "osdvar.h"
#include "fonts.h"
#include "UAVObj.h"
#include "osdconfig.h"
#include "math3d.h"
#include "px4_custom_mode.h"

#define R2D     57.295779513082320876798154814105f                                      //180/PI
#define D2R     0.017453292519943295769236907684886f                                    //PI/180

#define HUD_VSCALE_FLAG_CLEAR       1
#define HUD_VSCALE_FLAG_NO_NEGATIVE 2

int32_t test_alt, test_speed, test_throttle;

//2:small, 0:normal, 3:large
const int SIZE_TO_FONT[3] = { 2, 0, 3 };

uint8_t last_panel = 1;
uint64_t new_panel_start_time = 0;

uint8_t last_warn_type = 0;
uint64_t last_warn_time = 0;
char warn_str[256];

const char METRIC_SPEED[] = "km/h";         //kilometer per hour
const char METRIC_DIST_SHORT[] = "m";       //meter
const char METRIC_DIST_LONG[] = "km";       //kilometer

const char IMPERIAL_SPEED[] = "mph";        //mile per hour
const char IMPERIAL_DIST_SHORT[] = "ft";     //feet
const char IMPERIAL_DIST_LONG[] = "ml";      //mile

// Unit conversion constants
float convert_speed = 0.0f;
float convert_distance = 0.0f;
float convert_distance_divider = 0.0f;
const char * dist_unit_short = METRIC_DIST_SHORT;
const char * dist_unit_long = METRIC_DIST_LONG;
const char * spd_unit = METRIC_SPEED;


uint64_t GetSystimeMS(void) {
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    uint64_t milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000; // caculate milliseconds
    return milliseconds;
}


void osd_init(int shift_x, int shift_y, float scale_x, float scale_y)
{
    sys_start_time = GetSystimeMS();
    render_init(shift_x, shift_y, scale_x, scale_y);
    atti_mp_scale = (float)osd_params.Atti_mp_scale_real + (float)osd_params.Atti_mp_scale_frac * 0.01;
    atti_3d_scale = (float)osd_params.Atti_3D_scale_real + (float)osd_params.Atti_3D_scale_frac * 0.01;
    atti_3d_min_clipX = osd_params.Atti_mp_posX - (uint32_t)(22 * atti_mp_scale);
    atti_3d_max_clipX = osd_params.Atti_mp_posX + (uint32_t)(22 * atti_mp_scale);
    atti_3d_min_clipY = osd_params.Atti_mp_posY - (uint32_t)(30 * atti_mp_scale);
    atti_3d_max_clipY = osd_params.Atti_mp_posY + (uint32_t)(34 * atti_mp_scale);

    Build_Sin_Cos_Tables();
    uav2D_init();
    simple_attitude_init();
    home_direction_init();
}


void do_converts(void) {
  if (osd_params.Units_mode == 1)
  {
    convert_distance = 3.28f;
    convert_speed = 2.23f;
    convert_distance_divider = 5280.0f;     // feet in a mile
    dist_unit_short = IMPERIAL_DIST_SHORT;
    dist_unit_long = IMPERIAL_DIST_LONG;
    spd_unit = IMPERIAL_SPEED;
  }
  else
  {
    convert_distance = 1.0f;
    convert_speed = 3.6f;
    convert_distance_divider = 1000.0f;
    dist_unit_short = METRIC_DIST_SHORT;
    dist_unit_long = METRIC_DIST_LONG;
    spd_unit = METRIC_SPEED;
  }
}

bool shownAtPanel(uint16_t itemPanel) {
  //issue #1 - fixed
  return ((itemPanel & (1 << (current_panel - 1))) != 0);
}

bool enabledAndShownOnPanel(uint16_t enabled, uint16_t panel) {
  return enabled == 1 && shownAtPanel(panel);
}


void render() {
    clearGraphics();
    RenderScreen();
    displayGraphics();
}

// TODO: try if this is performance critical or not
char tmp_str[50] = { 0 };

void RenderScreen(void) {
  do_converts();

  if (current_panel > osd_params.Max_panels) {
    current_panel = 1;
  }

  draw_flight_mode();
  draw_arm_state();
  draw_battery_voltage();
  draw_battery_current();
  draw_battery_remaining();
  draw_battery_consumed();
  draw_altitude_scale();
  draw_absolute_altitude();
  draw_relative_altitude();
  draw_speed_scale();
  //draw_vtol_speed();
  draw_ground_speed();
  //draw_air_speed();
  draw_home_direction();
  draw_uav2d();
  draw_throttle();
  draw_home_latitude();
  draw_home_longitude();
  draw_gps_status();
  draw_gps_hdop();
  draw_gps_latitude();
  draw_gps_longitude();
  draw_gps2_status();
  draw_gps2_hdop();
  draw_gps2_latitude();
  draw_gps2_longitude();
  draw_total_trip();
  draw_time();
  draw_CWH();
  draw_climb_rate();
  draw_rssi();
  draw_wfb_state();
  draw_link_quality();
  draw_efficiency();
  draw_wind();

  draw_panel_changed();
  draw_warning();
}


void draw_simple_attitude() {
  Reset_Polygon2D(&simple_attitude);
  const int radius = 4 * atti_mp_scale;

  int x = simple_attitude.x0;
  int y = simple_attitude.y0;

  write_line_outlined(x - radius - 1, y, x - 2 * radius - 1, y, 0, 0, 0, 1);
  write_line_outlined(x + radius - 1, y, x + 2 * radius + 1, y, 0, 0, 0, 1);
  write_line_outlined(x, y - radius - 1, x, y - 2 * radius, 0, 0, 0, 1);
  write_circle_outlined(x, y, radius, 0, 1, 0, 1);

  Transform_Polygon2D(&simple_attitude, -osd_roll, 0, osd_pitch);

  for (int i = 0; i < simple_attitude.num_verts; i += 2) {
    write_line_outlined(simple_attitude.vlist_trans[i].x + x, simple_attitude.vlist_trans[i].y + y,
                        simple_attitude.vlist_trans[i + 1].x + x, simple_attitude.vlist_trans[i + 1].y + y,
                        2, 2, 0, 1);
  }

  //draw pitch value
  y = simple_attitude.y0 + 15;
  snprintf(tmp_str, sizeof(tmp_str), "PT %d", (int)osd_pitch);
  write_string(tmp_str, x, y + 5, 0, 0, TEXT_VA_TOP, TEXT_HA_CENTER, 0, SIZE_TO_FONT[1]);

  //draw roll value
  y = simple_attitude.y0 - 20;

  snprintf(tmp_str, sizeof(tmp_str), "RL %d", (int)osd_roll);
  write_string(tmp_str, x, y - 3, 0, 0, TEXT_VA_BOTTOM, TEXT_HA_CENTER, 0, SIZE_TO_FONT[1]);

}


void draw_radar() {
  int index = 0;

  Reset_Polygon2D(&uav2D);
  Transform_Polygon2D(&uav2D, -osd_roll, 0, osd_pitch);

  // loop thru and draw a line from vertices 1 to n
  VECTOR4D v;
  for (index = 0; index < uav2D.num_verts - 1; )
  {
    VECTOR4D_INITXYZW(&v, uav2D.vlist_trans[index].x + uav2D.x0, uav2D.vlist_trans[index].y + uav2D.y0,
                      uav2D.vlist_trans[index + 1].x + uav2D.x0, uav2D.vlist_trans[index + 1].y + uav2D.y0);

    if (Clip_Line(&v))
    {
        write_line_outlined(v.x, v.y, v.z, v.w, 2, 2, 0, 1);
    }
    index += 2;
  }   // end for

  //rotate roll scale and display, we only cal x
  Reset_Polygon2D(&rollscale2D);
  Rotate_Polygon2D(&rollscale2D, -osd_roll);
  for (index = 0; index < rollscale2D.num_verts - 1; index++)
  {
    // draw line from ith to ith+1 vertex
    write_line_outlined(rollscale2D.vlist_trans[index].x + rollscale2D.x0, rollscale2D.vlist_trans[index].y + rollscale2D.y0,
                        rollscale2D.vlist_trans[index + 1].x + rollscale2D.x0, rollscale2D.vlist_trans[index + 1].y + rollscale2D.y0,
                        2, 2, 0, 1);
  }   // end for

  int x = osd_params.Atti_mp_posX;
  int y = osd_params.Atti_mp_posY;
  int wingStart = (int)(12.0f * atti_mp_scale);
  int wingEnd = (int)(7.0f * atti_mp_scale);

  //draw uav
  write_line_outlined(x, y, x - 9, y + 5, 2, 2, 0, 1);
  write_line_outlined(x, y, x + 9, y + 5, 2, 2, 0, 1);
  write_line_outlined(x - wingStart, y, x - wingEnd, y, 2, 2, 0, 1);
  write_line_outlined(x + wingEnd, y, x + wingStart, y, 2, 2, 0, 1);

  write_filled_rectangle_lm(x - 9, y + 6, 15, 9, 0, 1);
  snprintf(tmp_str, sizeof(tmp_str), "%d", (int)osd_pitch);
  write_string(tmp_str, x, y + 5, 0, 0, TEXT_VA_TOP, TEXT_HA_CENTER, 0, SIZE_TO_FONT[1]);

  y = osd_params.Atti_mp_posY - (int)(38.0f * atti_mp_scale);
  //draw roll value
  write_line_outlined(x, y, x - 4, y + 8, 2, 2, 0, 1);
  write_line_outlined(x, y, x + 4, y + 8, 2, 2, 0, 1);
  write_line_outlined(x - 4, y + 8, x + 4, y + 8, 2, 2, 0, 1);
  snprintf(tmp_str, sizeof(tmp_str), "%d", (int)osd_roll);
  write_string(tmp_str, x, y - 3, 0, 0, TEXT_VA_BOTTOM, TEXT_HA_CENTER, 0, SIZE_TO_FONT[1]);
}

void draw_home_direction() {
  if (!enabledAndShownOnPanel(osd_params.HomeDirection_enabled,
                              osd_params.HomeDirection_panel)) {
    return;
  }
  float bearing = osd_home_bearing - osd_heading;
  Reset_Polygon2D(&home_direction);
  Reset_Polygon2D(&home_direction_outline);
  Rotate_Polygon2D(&home_direction, bearing);
  Rotate_Polygon2D(&home_direction_outline, bearing);

  const int x = home_direction.x0;
  const int y = home_direction.y0;


  for (int i = 0; i < home_direction.num_verts; i += 2) {
    write_line_lm(home_direction.vlist_trans[i].x + x,
                  home_direction.vlist_trans[i].y + y,
                  home_direction.vlist_trans[i + 1].x + x,
                  home_direction.vlist_trans[i + 1].y + y,
                  1, 1);
  }

  for (int i = 0; i < home_direction.num_verts; i += 2) {
    write_line_lm(home_direction_outline.vlist_trans[i].x + x,
                  home_direction_outline.vlist_trans[i].y + y,
                  home_direction_outline.vlist_trans[i + 1].x + x,
                  home_direction_outline.vlist_trans[i + 1].y + y,
                  1, 0);
  }
}

void draw_uav2d() {
  if (!enabledAndShownOnPanel(osd_params.Atti_mp_en,
                              osd_params.Atti_mp_panel)) {
    return;
  }
 
  if (osd_params.Atti_mp_type == 0) {
    draw_radar();
  } else {
    draw_simple_attitude();
  }
}

void draw_throttle(void) {
  if (!enabledAndShownOnPanel(osd_params.Throt_en,
                              osd_params.Throt_panel)) {
      return;
  }

  int16_t pos_th_y, pos_th_x;
  int posX, posY;
  posX = osd_params.Throt_posX;
  posY = osd_params.Throt_posY;

  pos_th_y = (int16_t)(0.5 * osd_throttle);
  pos_th_x = posX - 25 + pos_th_y;
  snprintf(tmp_str, sizeof(tmp_str), "%d%%", (int32_t)osd_throttle);
  write_string(tmp_str, posX, posY, 0, 0, TEXT_VA_TOP, TEXT_HA_RIGHT, 0, SIZE_TO_FONT[0]);

  if (osd_params.Throt_scale_en) {
    pos_th_y = (int16_t)(0.5 * osd_throttle);
    pos_th_x = posX - 25 + pos_th_y;
    snprintf(tmp_str, sizeof(tmp_str), "%d%%", (int32_t)osd_throttle);
    write_string(tmp_str, posX, posY, 0, 0, TEXT_VA_TOP, TEXT_HA_RIGHT, 0, SIZE_TO_FONT[0]);
    if (osd_params.Throttle_Scale_Type == 0) {
      write_filled_rectangle_lm(posX + 3, posY + 25 - pos_th_y, 5, pos_th_y, 1, 1);
      write_hline_lm(posX + 3, posX + 7, posY - 25, 1, 1);
      write_hline_lm(posX + 3, posX + 7, posY + 25 - pos_th_y, 1, 1);
      write_vline_lm(posX + 3, posY - 25, posY + 25 - pos_th_y, 1, 1);
      write_vline_lm(posX + 7, posY - 25, posY + 25 - pos_th_y, 1, 1);
    }
    else if (osd_params.Throttle_Scale_Type == 1) {
      write_filled_rectangle_lm(posX - 25, posY + 10, pos_th_y, 5, 1, 1);
      write_hline_lm(pos_th_x, posX + 25, posY + 10, 1, 1);
      write_hline_lm(pos_th_x, posX + 25, posY + 14, 1, 1);
      write_vline_lm(posX + 25, posY + 10, posY + 14, 1, 1);
      write_vline_lm(posX - 25, posY + 10, posY + 14, 1, 1);
    }
  } else {
    pos_th_y = (int16_t)(0.5 * osd_throttle);
    snprintf(tmp_str, sizeof(tmp_str), "THR %3d%%", (int32_t)osd_throttle);
    write_string(tmp_str, posX, posY, 0, 0, TEXT_VA_TOP, TEXT_HA_RIGHT, 0, SIZE_TO_FONT[0]);
  }
}

void draw_home_latitude() {
  if (!enabledAndShownOnPanel(osd_params.HomeLatitude_enabled,
                              osd_params.HomeLatitude_panel)) {
    return;
  }

  snprintf(tmp_str, sizeof(tmp_str), "H %0.5f", (double) osd_home_lat / 10000000.0f);
  write_string(tmp_str, osd_params.HomeLatitude_posX,
               osd_params.HomeLatitude_posY, 0, 0, TEXT_VA_TOP,
               osd_params.HomeLatitude_align, 0,
               SIZE_TO_FONT[osd_params.HomeLatitude_fontsize]);
}

void draw_home_longitude() {
  if (!enabledAndShownOnPanel(osd_params.HomeLongitude_enabled,
                              osd_params.HomeLongitude_panel)) {
    return;
  }

  snprintf(tmp_str, sizeof(tmp_str), "H %0.5f", (double) osd_home_lon / 10000000.0f);
  write_string(tmp_str, osd_params.HomeLongitude_posX,
               osd_params.HomeLongitude_posY, 0, 0, TEXT_VA_TOP,
               osd_params.HomeLongitude_align, 0,
               SIZE_TO_FONT[osd_params.HomeLongitude_fontsize]);
}

void draw_gps_status() {
  if (!enabledAndShownOnPanel(osd_params.GpsStatus_en,
                              osd_params.GpsStatus_panel)) {
    return;
  }

  switch (osd_fix_type) {
  case NO_GPS:
  case NO_FIX:
    snprintf(tmp_str, sizeof(tmp_str), "NOFIX");
    break;
  case GPS_OK_FIX_2D:
    snprintf(tmp_str, sizeof(tmp_str), "2D-%d", (int) osd_satellites_visible);
    break;
  case GPS_OK_FIX_3D:
    snprintf(tmp_str, sizeof(tmp_str), "3D-%d", (int) osd_satellites_visible);
    break;
  case GPS_OK_FIX_3D_DGPS:
    snprintf(tmp_str, sizeof(tmp_str), "D3D-%d", (int) osd_satellites_visible);
    break;
  default:
    snprintf(tmp_str, sizeof(tmp_str), "NOGPS");
    break;
  }
  write_string(tmp_str, osd_params.GpsStatus_posX,
               osd_params.GpsStatus_posY, 0, 0, TEXT_VA_TOP,
               osd_params.GpsStatus_align, 0,
               SIZE_TO_FONT[osd_params.GpsStatus_fontsize]);
}

void draw_gps_hdop() {
  if (!enabledAndShownOnPanel(osd_params.GpsHDOP_en,
                              osd_params.GpsHDOP_panel)) {
    return;
  }

  snprintf(tmp_str, sizeof(tmp_str), "HDOP %0.1f", (double) osd_hdop / 100.0f);
  write_string(tmp_str, osd_params.GpsHDOP_posX,
               osd_params.GpsHDOP_posY, 0, 0, TEXT_VA_TOP,
               osd_params.GpsHDOP_align, 0,
               SIZE_TO_FONT[osd_params.GpsHDOP_fontsize]);
}

void draw_gps_latitude() {
  if (!enabledAndShownOnPanel(osd_params.GpsLat_en,
                              osd_params.GpsLat_panel)) {
    return;
  }

  snprintf(tmp_str, sizeof(tmp_str), "%0.5f", (double) osd_lat / 10000000.0f);
  write_string(tmp_str, osd_params.GpsLat_posX,
               osd_params.GpsLat_posY, 0, 0, TEXT_VA_TOP,
               osd_params.GpsLat_align, 0,
               SIZE_TO_FONT[osd_params.GpsLat_fontsize]);
}

void draw_gps_longitude() {
  if (!enabledAndShownOnPanel(osd_params.GpsLon_en,
                              osd_params.GpsLon_panel)) {
    return;
  }

  snprintf(tmp_str, sizeof(tmp_str), "%0.5f", (double) osd_lon / 10000000.0f);
  write_string(tmp_str, osd_params.GpsLon_posX,
               osd_params.GpsLon_posY, 0, 0, TEXT_VA_TOP,
               osd_params.GpsLon_align, 0,
               SIZE_TO_FONT[osd_params.GpsLon_fontsize]);
}

void draw_gps2_status() {
  if (!enabledAndShownOnPanel(osd_params.Gps2Status_en,
                              osd_params.Gps2Status_panel)) {
    return;
  }

  switch (osd_fix_type2) {
  case NO_GPS:
  case NO_FIX:
    snprintf(tmp_str, sizeof(tmp_str), "NOFIX");
    break;
  case GPS_OK_FIX_2D:
    snprintf(tmp_str, sizeof(tmp_str), "2D-%d", (int) osd_satellites_visible2);
    break;
  case GPS_OK_FIX_3D:
    snprintf(tmp_str, sizeof(tmp_str), "3D-%d", (int) osd_satellites_visible2);
    break;
  case GPS_OK_FIX_3D_DGPS:
    snprintf(tmp_str, sizeof(tmp_str), "D3D-%d", (int) osd_satellites_visible2);
    break;
  default:
    snprintf(tmp_str, sizeof(tmp_str), "NOGPS");
    break;
  }
  write_string(tmp_str, osd_params.Gps2Status_posX,
               osd_params.Gps2Status_posY, 0, 0, TEXT_VA_TOP,
               osd_params.Gps2Status_align, 0,
               SIZE_TO_FONT[osd_params.Gps2Status_fontsize]);
}

void draw_gps2_hdop() {
  if (!enabledAndShownOnPanel(osd_params.Gps2HDOP_en,
                              osd_params.Gps2HDOP_panel)) {
    return;
  }

  snprintf(tmp_str, sizeof(tmp_str), "HDOP %0.1f", (double) osd_hdop2 / 100.0f);
  write_string(tmp_str, osd_params.Gps2HDOP_posX,
               osd_params.Gps2HDOP_posY, 0, 0, TEXT_VA_TOP,
               osd_params.Gps2HDOP_align, 0,
               SIZE_TO_FONT[osd_params.Gps2HDOP_fontsize]);
}

void draw_gps2_latitude() {
  if (!enabledAndShownOnPanel(osd_params.Gps2Lat_en,
                              osd_params.Gps2Lat_panel)) {
    return;
  }

  snprintf(tmp_str, sizeof(tmp_str), "%0.5f", (double) osd_lat2 / 10000000.0f);
  write_string(tmp_str, osd_params.Gps2Lat_posX,
               osd_params.Gps2Lat_posY, 0, 0, TEXT_VA_TOP,
               osd_params.Gps2Lat_align, 0,
               SIZE_TO_FONT[osd_params.Gps2Lat_fontsize]);
}

void draw_gps2_longitude() {
  if (!enabledAndShownOnPanel(osd_params.Gps2Lon_en,
                              osd_params.Gps2Lon_panel)) {
    return;
  }

  snprintf(tmp_str, sizeof(tmp_str), "%0.5f", (double) osd_lon2 / 10000000.0f);
  write_string(tmp_str, osd_params.Gps2Lon_posX,
               osd_params.Gps2Lon_posY, 0, 0, TEXT_VA_TOP,
               osd_params.Gps2Lon_align, 0,
               SIZE_TO_FONT[osd_params.Gps2Lon_fontsize]);
}

void draw_total_trip() {
  if (!enabledAndShownOnPanel(osd_params.TotalTripDist_en,
                              osd_params.TotalTripDist_panel)) {
    return;
  }

  float tmp = osd_total_trip_dist * convert_distance;
  if (tmp < convert_distance_divider) {
    snprintf(tmp_str, sizeof(tmp_str), "%d%s", (int) tmp, dist_unit_short);
  }
  else{
    snprintf(tmp_str, sizeof(tmp_str), "%0.2f%s", (double) (tmp / convert_distance_divider), dist_unit_long);
  }
  write_string(tmp_str, osd_params.TotalTripDist_posX,
               osd_params.TotalTripDist_posY, 0, 0, TEXT_VA_TOP,
               osd_params.TotalTripDist_align, 0,
               SIZE_TO_FONT[osd_params.TotalTripDist_fontsize]);
}

void draw_time() {
  if (!enabledAndShownOnPanel(osd_params.Time_en,
                              osd_params.Time_panel)) {
    return;
  }

  time_t t = time(NULL);
  struct tm *lt = localtime(&t);

  if (lt == NULL){
      return;
  }

  strftime(tmp_str, sizeof(tmp_str), "%H:%M:%S", lt);
  write_string(tmp_str, osd_params.Time_posX,
               osd_params.Time_posY, 0, 0, TEXT_VA_TOP,
               osd_params.Time_align, 0,
               SIZE_TO_FONT[osd_params.Time_fontsize]);
}

void draw_CWH(void) {
  char tmp_str[100] = { 0 };
  float dstlon, dstlat, dstsqrt;

  if ((osd_got_home == 0) && (motor_armed) && (osd_fix_type > 1)) {
    osd_home_lat = osd_lat;
    osd_home_lon = osd_lon;
    osd_alt_cnt = 0;
    osd_got_home = 1;
  }
  else if (osd_got_home == 1) {
    // JRChange: osd_home_alt: check for stable osd_alt (must be stable for 75*40ms = 3s)
    // we can get the relative alt from mavlink directly.
    if (osd_alt_cnt < 75) {
      if (fabs(osd_alt_prev - osd_alt) > 0.5) {
        osd_alt_cnt = 0;
        osd_alt_prev = osd_alt;
      } else {
        if (++osd_alt_cnt >= 75) {
          osd_home_alt = osd_alt;           // take this stable osd_alt as osd_home_alt
        }
      }
    }

    // shrinking factor for longitude going to poles direction
    double scaleLongDown = Fast_Cos(fabs(osd_home_lat));
    double scaleLongUp   = 1.0f / Fast_Cos(fabs(osd_home_lat));

    //DST to Home
    dstlat = fabs(osd_home_lat - osd_lat) * 111319.5f;
    dstlon = fabs(osd_home_lon - osd_lon) * 111319.5f * scaleLongDown;
    dstsqrt = dstlat * dstlat + dstlon * dstlon;
    osd_home_distance = sqrt(dstsqrt) / 10000000.0f;

    //DIR to Home
    dstlon = (osd_home_lon - osd_lon);     //OffSet_X
    dstlat = (osd_home_lat - osd_lat) * scaleLongUp;     //OffSet Y
    osd_home_bearing = 270 + (atan2(dstlat, -dstlon) * R2D);     //absolut home direction
    //osd_home_bearing = (osd_home_bearing+360)%360;
    if (osd_home_bearing > 360)
      osd_home_bearing -= 360;
  }

  //distance
  if (osd_params.CWH_home_dist_en == 1 && shownAtPanel(osd_params.CWH_home_dist_panel)) {
    float tmp = osd_home_distance * convert_distance;
    if (tmp < convert_distance_divider)
      snprintf(tmp_str, sizeof(tmp_str), "H: %d%s", (int)tmp, dist_unit_short);
    else
      snprintf(tmp_str, sizeof(tmp_str), "H: %0.2f%s", (double)(tmp / convert_distance_divider), dist_unit_long);

    write_string(tmp_str, osd_params.CWH_home_dist_posX, osd_params.CWH_home_dist_posY, 0, 0, TEXT_VA_TOP, osd_params.CWH_home_dist_align, 0, SIZE_TO_FONT[osd_params.CWH_home_dist_fontsize]);
  }
  if ((wp_number != 0) && (osd_params.CWH_wp_dist_en) && shownAtPanel(osd_params.CWH_wp_dist_panel)) {
    float tmp = wp_dist * convert_distance;
    if (tmp < convert_distance_divider)
      snprintf(tmp_str, sizeof(tmp_str), "WP %d%s", (int)tmp, dist_unit_short);
    else
      snprintf(tmp_str, sizeof(tmp_str), "WP %0.2f%s", (double)(tmp / convert_distance_divider), dist_unit_long);

    write_string(tmp_str, osd_params.CWH_wp_dist_posX, osd_params.CWH_wp_dist_posY, 0, 0, TEXT_VA_TOP, osd_params.CWH_wp_dist_align, 0, SIZE_TO_FONT[osd_params.CWH_wp_dist_fontsize]);
  }

  //direction - map-like mode
  if (osd_params.CWH_Nmode_en == 1 && shownAtPanel(osd_params.CWH_Nmode_panel)) {
      draw_head_wp_home();
  }

  //direction - scale mode
  if (osd_params.CWH_Tmode_en == 1 && shownAtPanel(osd_params.CWH_Tmode_panel)) {
      draw_linear_compass(osd_heading, 0, 120, 180, GRAPHICS_X_MIDDLE, osd_params.CWH_Tmode_posY, 15, 30, 5, 8, 0);
  }
}

void draw_climb_rate() {
  if (!enabledAndShownOnPanel(osd_params.ClimbRate_en,
                              osd_params.ClimbRate_panel)) {
    return;
  }

  float average_climb = 0.0f;
  osd_climb_ma[osd_climb_ma_index] = osd_climb;
  osd_climb_ma_index = (osd_climb_ma_index + 1) % 10;

  for (int i = 0; i < 10; i++) {
    average_climb = average_climb + osd_climb_ma[i];
  }

  average_climb = roundf(10 * (average_climb / 10)) / 10.0f;

  int x = osd_params.ClimbRate_posX;
  int y = osd_params.ClimbRate_posY;

  if(average_climb == 0.0f)
  {
     return;
  }
  else if(fabs(average_climb) < 10.0f)
  {
      snprintf(tmp_str, sizeof(tmp_str), "%.1f", fabs(average_climb));
  }
  else
  {
      snprintf(tmp_str, sizeof(tmp_str), "%.0f", fabs(average_climb));
  }
 
  write_string(tmp_str, x - 5, y, 0, 0, TEXT_VA_MIDDLE, TEXT_HA_RIGHT, 0,
               SIZE_TO_FONT[osd_params.ClimbRate_fontsize]);

  int arrowLength = 6;
  if (osd_params.ClimbRate_fontsize != 0) {
    arrowLength += 2;
  }

  if (average_climb > 0.0f) {
    write_vline_lm(x, y - arrowLength, y + arrowLength, 1, 1);
    write_line_outlined(x - 3, y - arrowLength + 3, x, y - arrowLength, 2, 2, 0, 1);
    write_line_outlined(x + 3, y - arrowLength + 3, x, y - arrowLength, 2, 2, 0, 1);
  } else if (average_climb < 0.0f) {
    write_vline_lm(x, y - arrowLength, y + arrowLength, 1, 1);
    write_line_outlined(x - 3, y + arrowLength - 3, x, y + arrowLength, 2, 2, 0, 1);
    write_line_outlined(x + 3, y + arrowLength - 3, x, y + arrowLength, 2, 2, 0, 1);
  }
}

void draw_rssi() {
  if (!enabledAndShownOnPanel(osd_params.RSSI_en,
                              osd_params.RSSI_panel)) {
    return;
  }

  int rssi = (int)osd_rssi;

  //Not from the MAVLINK, should take the RC channel PWM value.
  if (osd_params.RSSI_type != 0)
  {
    if (osd_params.RSSI_type == 5) rssi = (int)osd_chan5_raw;
    else if (osd_params.RSSI_type == 6) rssi = (int)osd_chan6_raw;
    else if (osd_params.RSSI_type == 7) rssi = (int)osd_chan7_raw;
    else if (osd_params.RSSI_type == 8) rssi = (int)osd_chan8_raw;
    else if (osd_params.RSSI_type == 9) rssi = (int)osd_chan9_raw;
    else if (osd_params.RSSI_type == 10) rssi = (int)osd_chan10_raw;
    else if (osd_params.RSSI_type == 11) rssi = (int)osd_chan11_raw;
    else if (osd_params.RSSI_type == 12) rssi = (int)osd_chan12_raw;
    else if (osd_params.RSSI_type == 13) rssi = (int)osd_chan13_raw;
    else if (osd_params.RSSI_type == 14) rssi = (int)osd_chan14_raw;
    else if (osd_params.RSSI_type == 15) rssi = (int)osd_chan15_raw;
    else if (osd_params.RSSI_type == 16) rssi = (int)osd_chan16_raw;
  }

  //0:percentage 1:raw
  if ((osd_params.RSSI_raw_en == 0)) {
    uint16_t rssiMin = osd_params.RSSI_min;
    uint16_t rssiMax = osd_params.RSSI_max;

    //Trim the rssiMAX/MIN only for MAVLINK-rssi
    if (osd_params.RSSI_type == 0) {
      if (rssiMin < 0)
        rssiMin = 0;
      if (rssiMax > 255)
        rssiMax = 255;
    }

    if ((rssiMax - rssiMin) > 0)
      rssi = (int) ((float) (rssi - rssiMin) / (float) (rssiMax - rssiMin) * 100.0f);

    if (rssi < 0) rssi = 0;
    snprintf(tmp_str, sizeof(tmp_str), "RC: %d%%", rssi);
    rc_lost = (rssi < 5) ? true : false;
  }
  else
  {
    snprintf(tmp_str, sizeof(tmp_str), "RC: %d", rssi);
    rc_lost = false;
  }

  write_string(tmp_str, osd_params.RSSI_posX,
               osd_params.RSSI_posY, 0, 0, TEXT_VA_TOP,
               osd_params.RSSI_align, 0,
               SIZE_TO_FONT[osd_params.RSSI_fontsize]);
}

void draw_link_quality() {
  if (!enabledAndShownOnPanel(osd_params.LinkQuality_en,
                              osd_params.LinkQuality_panel)) {
    return;
  }

  int linkquality = (int)linkquality;
  int min = osd_params.LinkQuality_min;
  int max = osd_params.LinkQuality_max;

  if (osd_params.LinkQuality_chan == 5) linkquality = (int)osd_chan5_raw;
  else if (osd_params.LinkQuality_chan == 6) linkquality = (int)osd_chan6_raw;
  else if (osd_params.LinkQuality_chan == 7) linkquality = (int)osd_chan7_raw;
  else if (osd_params.LinkQuality_chan == 8) linkquality = (int)osd_chan8_raw;
  else if (osd_params.LinkQuality_chan == 9) linkquality = (int)osd_chan9_raw;
  else if (osd_params.LinkQuality_chan == 10) linkquality = (int)osd_chan10_raw;
  else if (osd_params.LinkQuality_chan == 11) linkquality = (int)osd_chan11_raw;
  else if (osd_params.LinkQuality_chan == 12) linkquality = (int)osd_chan12_raw;
  else if (osd_params.LinkQuality_chan == 13) linkquality = (int)osd_chan13_raw;
  else if (osd_params.LinkQuality_chan == 14) linkquality = (int)osd_chan14_raw;
  else if (osd_params.LinkQuality_chan == 15) linkquality = (int)osd_chan15_raw;
  else if (osd_params.LinkQuality_chan == 16) linkquality = (int)osd_chan16_raw;

  // 0: percent, 1: raw
  if (osd_params.LinkQuality_type == 0) {
    //OpenLRS will output 0 instead of min if the RX is powerd up before the TX
    if (linkquality < min)
    {
      linkquality = min;
    }

    //Get rid of any variances
    if (linkquality > max)
    {
      linkquality = max;
    }

    //Funky Conversion from  pwm min & max to percent
    linkquality = (int) ((float) (linkquality - max) / (float) (max - min) * 100.0f) + 100;
    snprintf(tmp_str, sizeof(tmp_str), "LIQU %d%%", linkquality);
  } else {
    snprintf(tmp_str, sizeof(tmp_str), "LIQU %d", linkquality);
  }

  write_string(tmp_str, osd_params.LinkQuality_posX,
               osd_params.LinkQuality_posY, 0, 0, TEXT_VA_MIDDLE,
               osd_params.LinkQuality_align, 0,
               SIZE_TO_FONT[osd_params.LinkQuality_fontsize]);
}

void draw_efficiency() {
  if (!enabledAndShownOnPanel(osd_params.Efficiency_en,
                              osd_params.Efficiency_panel)) {
    return;
  }

  float wattage = osd_vbat_A * osd_curr_A * 0.01;
  float speed = osd_groundspeed * convert_speed;
  float efficiency = 0;
  if (speed != 0) {
    efficiency = wattage / speed;
  }
  snprintf(tmp_str, sizeof(tmp_str), "%0.1fW/%s", efficiency, dist_unit_long);

  write_string(tmp_str, osd_params.Efficiency_posX, osd_params.Efficiency_posY,
               0, 0, TEXT_VA_TOP, osd_params.Efficiency_align, 0,
               SIZE_TO_FONT[osd_params.Efficiency_fontsize]);
}

void draw_panel_changed() {
  if (last_panel != current_panel) {
    last_panel = current_panel;
    new_panel_start_time = GetSystimeMS();
  }

  if ((GetSystimeMS() - new_panel_start_time) < 3000) {
    snprintf(tmp_str, sizeof(tmp_str), "P %d", (int) current_panel);
    write_string(tmp_str, GRAPHICS_X_MIDDLE, 210, 0, 0, TEXT_VA_TOP,
                 TEXT_HA_CENTER, 0, SIZE_TO_FONT[1]);
  }
}

/**
 * hud_draw_compass: Draw a compass.
 *
 * @param       v               value for the compass
 * @param       range           range about value to display (+/- range/2 each direction)
 * @param       width           length in pixels
 * @param       x               x displacement
 * @param       y               y displacement
 * @param       mintick_step    how often a minor tick is shown
 * @param       majtick_step    how often a major tick (heading "xx") is shown
 * @param       mintick_len     minor tick length
 * @param       majtick_len     major tick length
 * @param       flags           special flags (see hud.h.)
 */
#define COMPASS_SMALL_NUMBER
// #define COMPASS_FILLED_NUMBER
void draw_linear_compass(int v, int home_dir, int range, int width, int x, int y, int mintick_step, int majtick_step, int mintick_len, int majtick_len, __attribute__((unused)) int flags) {
  v %= 360;   // wrap, just in case.
  struct FontEntry font_info;
  int majtick_start = 0, majtick_end = 0, mintick_start = 0, mintick_end = 0, textoffset = 0;
  char headingstr[4];
  majtick_start = y;
  majtick_end   = y - majtick_len;
  mintick_start = y;
  mintick_end   = y - mintick_len;
  textoffset    = 8;
  int r, style, rr, xs;   // rv,
  int range_2 = range / 2;
  bool home_drawn = false;

  // v = 30;
  // home_dir = 30;
  // int wp_dir = 60;

  for (r = -range_2; r <= +range_2; r++) {
    style = 0;
    rr    = (v + r + 360) % 360;     // normalise range for modulo, add to move compass track
    // rv = -rr + range_2; // for number display
    if (rr % majtick_step == 0) {
      style = 1;       // major tick
    } else if (rr % mintick_step == 0) {
      style = 2;       // minor tick
    }
    if (style) {
      // Calculate x position.
      xs = ((long int)(r * width) / (long int)range) + x;
      // Draw it.
      if (style == 1) {
        write_vline_outlined(xs, majtick_start, majtick_end, 2, 2, 0, 1);
        // Draw heading above this tick.
        // If it's not one of north, south, east, west, draw the heading.
        // Otherwise, draw one of the identifiers.
        if (rr % 90 != 0) {
            snprintf(headingstr, sizeof(headingstr), "%d", rr);
        } else {
          switch (rr) {
          case 0:
            headingstr[0] = 'N';
            break;
          case 90:
            headingstr[0] = 'E';
            break;
          case 180:
            headingstr[0] = 'S';
            break;
          case 270:
            headingstr[0] = 'W';
            break;
          }
          headingstr[1] = 0;
          headingstr[2] = 0;
          headingstr[3] = 0;
        }
        // +1 fudge...!
        write_string(headingstr, xs + 1, majtick_start + textoffset, 1, 0, TEXT_VA_MIDDLE, TEXT_HA_CENTER, 0, 2);

      } else if (style == 2) {
        write_vline_outlined(xs, mintick_start, mintick_end, 2, 2, 0, 1);
      }
    }

     // Put home direction
     if (rr == home_dir) {
         xs = ((long int)(r * width) / (long int)range) + x;
         write_filled_rectangle_lm(xs - 5, majtick_start + textoffset + 7, 10, 10, 0, 1);
         write_string("H", xs + 1, majtick_start + textoffset + 12, 1, 0, TEXT_VA_MIDDLE, TEXT_HA_CENTER, 0, 2);
         home_drawn = true;
     }

     /* // Put home direction */
     /* if (rr == wp_dir) { */
     /*     xs = ((long int)(r * width) / (long int)range) + x; */
     /*     write_filled_rectangle_lm(xs - 5, majtick_start + textoffset + 7, 10, 10, 0, 1); */
     /*     write_string("W", xs + 1, majtick_start + textoffset + 12, 1, 0, TEXT_VA_MIDDLE, TEXT_HA_CENTER, 0, 1); */
     /*     home_drawn = true; */
     /* } */
  }

  if (home_dir > 0 && !home_drawn) {
     if (((v > home_dir) && (v - home_dir < 180)) || ((v < home_dir) && (home_dir -v > 180)))
         r = x - ((long int)(range_2 * width) / (long int)range);
     else
         r = x + ((long int)(range_2 * width) / (long int)range);

     write_filled_rectangle_lm(r - 5, majtick_start + textoffset + 7, 10, 10, 0, 1);
     write_string("H", r + 1, majtick_start + textoffset + 12, 1, 0, TEXT_VA_MIDDLE, TEXT_HA_CENTER, 0, 2);
  }


  // Then, draw a rectangle with the present heading in it.
  // We want to cover up any other markers on the bottom.
  // First compute font size.
  headingstr[0] = '0' + (v / 100);
  headingstr[1] = '0' + ((v / 10) % 10);
  headingstr[2] = '0' + (v % 10);
  headingstr[3] = 0;
  fetch_font_info(0, 3, &font_info, NULL);
#ifdef COMPASS_SMALL_NUMBER
  int rect_width = font_info.width * 3;
#ifdef COMPASS_FILLED_NUMBER
  write_filled_rectangle_lm(x - (rect_width / 2), majtick_start - 7, rect_width, font_info.height, 0, 1);
#else
  write_filled_rectangle_lm(x - (rect_width / 2), majtick_start - 7, rect_width, font_info.height, 0, 0);
#endif
  write_rectangle_outlined(x - (rect_width / 2), majtick_start - 7, rect_width, font_info.height, 0, 1);
  write_string(headingstr, x + 1, majtick_start + textoffset - 5, 0, 0, TEXT_VA_MIDDLE, TEXT_HA_CENTER, 1, 0);
#else
  int rect_width = (font_info.width + 1) * 3 + 2;
#ifdef COMPASS_FILLED_NUMBER
  write_filled_rectangle_lm(x - (rect_width / 2), majtick_start + 2, rect_width, font_info.height + 2, 0, 1);
#else
  write_filled_rectangle_lm(x - (rect_width / 2), majtick_start + 2, rect_width, font_info.height + 2, 0, 0);
#endif
  write_rectangle_outlined(x - (rect_width / 2), majtick_start + 2, rect_width, font_info.height + 2, 0, 1);
  write_string(headingstr, x + 1, majtick_start + textoffset + 2, 0, 0, TEXT_VA_MIDDLE, TEXT_HA_CENTER, 1, 3);
#endif
}

/**
 * draw_vertical_scale: Draw a vertical scale.
 *
 * @param       v                   value to display as an integer
 * @param       range               range about value to display (+/- range/2 each direction)
 * @param       halign              horizontal alignment: 0 = left, 1 = right.
 * @param       x                   x displacement
 * @param       y                   y displacement
 * @param       height              height of scale
 * @param       mintick_step        how often a minor tick is shown
 * @param       majtick_step        how often a major tick is shown
 * @param       mintick_len         minor tick length
 * @param       majtick_len         major tick length
 * @param       boundtick_len       boundary tick length
 * @param       max_val             maximum expected value (used to compute size of arrow ticker)
 * @param       flags               special flags (see hud.h.)
 */
// #define VERTICAL_SCALE_BRUTE_FORCE_BLANK_OUT
#define VERTICAL_SCALE_FILLED_NUMBER
void draw_vertical_scale(float v, int range, int halign, int x, int y,
                         int height, int mintick_step, int majtick_step, int mintick_len,
                         int majtick_len, int boundtick_len, __attribute__((unused)) int max_val,
                         int flags) {
  char temp[15];
  struct FontEntry font_info;
  struct FontDimensions dim;
  // Compute the position of the elements.
  int majtick_start = 0, majtick_end = 0, mintick_start = 0, mintick_end = 0, boundtick_start = 0, boundtick_end = 0;

  majtick_start   = x;
  mintick_start   = x;
  boundtick_start = x;
  if (halign == 0) {
    majtick_end     = x + majtick_len;
    mintick_end     = x + mintick_len;
    boundtick_end   = x + boundtick_len;
  } else if (halign == 1) {
    majtick_end     = x - majtick_len;
    mintick_end     = x - mintick_len;
    boundtick_end   = x - boundtick_len;
  }
  // Retrieve width of large font (font #0); from this calculate the x spacing.
  fetch_font_info(0, 0, &font_info, NULL);
  int arrow_len      = (font_info.height / 2) + 1;
  int text_x_spacing = (font_info.width / 2);
  int max_text_y     = 0, text_length = 0;
  int small_font_char_width = font_info.width + 1;   // +1 for horizontal spacing = 1
  // For -(range / 2) to +(range / 2), draw the scale.
  int range_2 = range / 2;   // , height_2 = height / 2;
  int r = 0, rr = 0, rv = 0, ys = 0, style = 0;   // calc_ys = 0,
  // Iterate through each step.
  for (r = -range_2; r <= +range_2; r++) {
    style = 0;
    rr    = r + range_2 - (int)v;     // normalise range for modulo, subtract value to move ticker tape
    rv    = -rr + range_2;     // for number display
    if (flags & HUD_VSCALE_FLAG_NO_NEGATIVE) {
      rr += majtick_step / 2;
    }
    if (rr % majtick_step == 0) {
      style = 1;       // major tick
    } else if (rr % mintick_step == 0) {
      style = 2;       // minor tick
    } else {
      style = 0;
    }
    if (flags & HUD_VSCALE_FLAG_NO_NEGATIVE && rv < 0) {
      continue;
    }
    if (style) {
      // Calculate y position.
      ys = ((long int)(r * height) / (long int)range) + y;
      // Depending on style, draw a minor or a major tick.
      if (style == 1) {
        write_hline_outlined(majtick_start, majtick_end, ys, 2, 2, 0, 1);
        memset(temp, ' ', 10);
        sprintf(temp, "%d", rv);
        text_length = (strlen(temp) + 1) * small_font_char_width;         // add 1 for margin
        if (text_length > max_text_y) {
          max_text_y = text_length;
        }
        if (halign == 0) {
          write_string(temp, majtick_end + text_x_spacing + 1, ys, 1, 0, TEXT_VA_MIDDLE, TEXT_HA_LEFT, 0, 1);
        } else {
          write_string(temp, majtick_end - text_x_spacing + 1, ys, 1, 0, TEXT_VA_MIDDLE, TEXT_HA_RIGHT, 0, 1);
        }
      } else if (style == 2) {
        write_hline_outlined(mintick_start, mintick_end, ys, 2, 2, 0, 1);
      }
    }
  }
  // Generate the string for the value, as well as calculating its dimensions.
  memset(temp, ' ', 10);
  // my_itoa(v, temp);
  if( v != 0.0f && fabsf(v) < 10.0f)
  {
      sprintf(temp, "%3.1f", v);
  }else
  {
      sprintf(temp, "%3d", (int)v);
  }
  // TODO: add auto-sizing.
  calc_text_dimensions(temp, font_info, 1, 0, &dim);
  dim.width += 3;

  int xx = 0, i = 0;
  if (halign == 0) {
    xx = majtick_end + text_x_spacing;
  } else {
    xx = majtick_end - text_x_spacing;
  }
  y++;
  // Draw an arrow from the number to the point.
  for (i = 0; i < arrow_len; i++) {
    if (halign == 0) {
      write_pixel_lm(xx - arrow_len + i, y - i - 1, 1, 1);
      write_pixel_lm(xx - arrow_len + i, y + i - 1, 1, 1);
#ifdef VERTICAL_SCALE_FILLED_NUMBER
      write_hline_lm(xx + dim.width - 1, xx - arrow_len + i + 1, y - i - 1, 0, 1);
      write_hline_lm(xx + dim.width - 1, xx - arrow_len + i + 1, y + i - 1, 0, 1);
#else
      write_hline_lm(xx + dim.width - 1, xx - arrow_len + i + 1, y - i - 1, 0, 0);
      write_hline_lm(xx + dim.width - 1, xx - arrow_len + i + 1, y + i - 1, 0, 0);
#endif
    } else {
      write_pixel_lm(xx + arrow_len - i, y - i - 1, 1, 1);
      write_pixel_lm(xx + arrow_len - i, y + i - 1, 1, 1);
#ifdef VERTICAL_SCALE_FILLED_NUMBER
      write_hline_lm(xx - dim.width - 1, xx + arrow_len - i - 1, y - i - 1, 0, 1);
      write_hline_lm(xx - dim.width - 1, xx + arrow_len - i - 1, y + i - 1, 0, 1);
#else
      write_hline_lm(xx - dim.width - 1, xx + arrow_len - i - 1, y - i - 1, 0, 0);
      write_hline_lm(xx - dim.width - 1, xx + arrow_len - i - 1, y + i - 1, 0, 0);
#endif
    }
  }
  if (halign == 0) {
    write_hline_lm(xx, xx + dim.width - 1, y - arrow_len, 1, 1);
    write_hline_lm(xx, xx + dim.width - 1, y + arrow_len - 2, 1, 1);
    write_vline_lm(xx + dim.width - 1, y - arrow_len, y + arrow_len - 2, 1, 1);
  } else {
    write_hline_lm(xx, xx - dim.width - 1, y - arrow_len, 1, 1);
    write_hline_lm(xx, xx - dim.width - 1, y + arrow_len - 2, 1, 1);
    write_vline_lm(xx - dim.width - 1, y - arrow_len, y + arrow_len - 2, 1, 1);
  }
  // Draw the text.
  if (halign == 0) {
    write_string(temp, xx, y, 1, 0, TEXT_VA_MIDDLE, TEXT_HA_LEFT, 0, 0);
  } else {
    write_string(temp, xx, y, 1, 0, TEXT_VA_MIDDLE, TEXT_HA_RIGHT, 0, 0);
  }
#ifdef VERTICAL_SCALE_BRUTE_FORCE_BLANK_OUT
  // This is a bad brute force method destuctive to other things that maybe drawn underneath like e.g. the artificial horizon:
  // Then, add a slow cut off on the edges, so the text doesn't sharply
  // disappear. We simply clear the areas above and below the ticker, and we
  // use little markers on the edges.
  if (halign == 0) {
    write_filled_rectangle_lm(majtick_end + text_x_spacing, y + (height / 2) - (font_info.height / 2), max_text_y - boundtick_start, font_info.height, 0, 0);
    write_filled_rectangle_lm(majtick_end + text_x_spacing, y - (height / 2) - (font_info.height / 2), max_text_y - boundtick_start, font_info.height, 0, 0);
  } else {
    write_filled_rectangle_lm(majtick_end - text_x_spacing - max_text_y, y + (height / 2) - (font_info.height / 2), max_text_y, font_info.height, 0, 0);
    write_filled_rectangle_lm(majtick_end - text_x_spacing - max_text_y, y - (height / 2) - (font_info.height / 2), max_text_y, font_info.height, 0, 0);
  }
#endif
  y--;
  write_hline_outlined(boundtick_start, boundtick_end, y + (height / 2), 2, 2, 0, 1);
  write_hline_outlined(boundtick_start, boundtick_end, y - (height / 2), 2, 2, 0, 1);
}

void draw_head_wp_home() {
  int posX, posY, r;
  char tmp_str[10] = { 0 };

  //draw compass
  posX = osd_params.CWH_Nmode_posX;
  posY = osd_params.CWH_Nmode_posY;
  r = osd_params.CWH_Nmode_radius;
  write_circle_outlined(posX, posY, r, 0, 1, 0, 1);
  write_string("N", posX, posY - r, 0, 0, TEXT_VA_TOP, TEXT_HA_CENTER, 0, SIZE_TO_FONT[0]);

  //draw heading
  POLYGON2D suav;
  suav.state       = 1;
  suav.num_verts   = 4;
  suav.x0          = posX;
  suav.y0          = posY;
  VECTOR2D_INITXYZ(&(suav.vlist_local[0]), 0, -14);
  VECTOR2D_INITXYZ(&(suav.vlist_local[1]), -6, 14);
  VECTOR2D_INITXYZ(&(suav.vlist_local[2]), 6, 14);
  VECTOR2D_INITXYZ(&(suav.vlist_local[3]), 0, 10);
  Reset_Polygon2D(&suav);
  Rotate_Polygon2D(&suav, osd_heading);

  write_line_outlined(suav.vlist_trans[0].x + suav.x0, suav.vlist_trans[0].y + suav.y0,
                      suav.vlist_trans[1].x + suav.x0, suav.vlist_trans[1].y + suav.y0, 2, 2, 0, 1);
  write_line_outlined(suav.vlist_trans[0].x + suav.x0, suav.vlist_trans[0].y + suav.y0,
                      suav.vlist_trans[2].x + suav.x0, suav.vlist_trans[2].y + suav.y0, 2, 2, 0, 1);
  write_line_outlined(suav.vlist_trans[3].x + suav.x0, suav.vlist_trans[3].y + suav.y0,
                      suav.vlist_trans[1].x + suav.x0, suav.vlist_trans[1].y + suav.y0, 2, 2, 0, 1);
  write_line_outlined(suav.vlist_trans[3].x + suav.x0, suav.vlist_trans[3].y + suav.y0,
                      suav.vlist_trans[2].x + suav.x0, suav.vlist_trans[2].y + suav.y0, 2, 2, 0, 1);

  // draw home
  // the home only shown when the distance above 1m
  if (((int32_t)osd_home_distance > 1))
  {
    float homeCX = posX + (osd_params.CWH_Nmode_home_radius) * Fast_Sin(osd_home_bearing);
    float homeCY = posY - (osd_params.CWH_Nmode_home_radius) * Fast_Cos(osd_home_bearing);
    write_string("H", homeCX, homeCY, 0, 0, TEXT_VA_MIDDLE, TEXT_HA_CENTER, 0, SIZE_TO_FONT[0]);
  }

  //draw waypoint
  if ((wp_number != 0) && (wp_dist > 1))
  {
    //format bearing
    wp_target_bearing = (wp_target_bearing + 360) % 360;
    float wpCX = posX + (osd_params.CWH_Nmode_wp_radius) * Fast_Sin(wp_target_bearing);
    float wpCY = posY - (osd_params.CWH_Nmode_wp_radius) * Fast_Cos(wp_target_bearing);
    snprintf(tmp_str, sizeof(tmp_str), "%d", (int)wp_number);
    write_string(tmp_str, wpCX, wpCY, 0, 0, TEXT_VA_MIDDLE, TEXT_HA_CENTER, 0, SIZE_TO_FONT[0]);
  }
}

void draw_wind(void) {
  if (!enabledAndShownOnPanel(osd_params.Wind_en,
                              osd_params.Wind_panel)) {
    return;
  }

  uint16_t posX = osd_params.Wind_posX;
  uint16_t posY = osd_params.Wind_posY;

  //write_string("wind:", posX, posY, 0, 0, TEXT_VA_MIDDLE, osd_params.Wind_align, 0, SIZE_TO_FONT[osd_params.Wind_fontsize]);

  //draw direction
  POLYGON2D obj2D;
  obj2D.state       = 1;
  obj2D.num_verts   = 5;
  obj2D.x0          = posX;
  obj2D.y0          = posY;
  VECTOR2D_INITXYZ(&(obj2D.vlist_local[0]), -3, -2);
  VECTOR2D_INITXYZ(&(obj2D.vlist_local[1]), 0, -8);
  VECTOR2D_INITXYZ(&(obj2D.vlist_local[2]), 3, -2);
  VECTOR2D_INITXYZ(&(obj2D.vlist_local[3]), 0, 8);
  VECTOR2D_INITXYZ(&(obj2D.vlist_local[4]), 0, -2);
  Reset_Polygon2D(&obj2D);
  Rotate_Polygon2D(&obj2D, osd_windDir);
  write_triangle_wire(obj2D.vlist_trans[0].x + obj2D.x0, obj2D.vlist_trans[0].y + obj2D.y0,
                      obj2D.vlist_trans[1].x + obj2D.x0, obj2D.vlist_trans[1].y + obj2D.y0,
                      obj2D.vlist_trans[2].x + obj2D.x0, obj2D.vlist_trans[2].y + obj2D.y0);
  write_line_outlined(obj2D.vlist_trans[3].x + obj2D.x0, obj2D.vlist_trans[3].y + obj2D.y0,
                      obj2D.vlist_trans[4].x + obj2D.x0, obj2D.vlist_trans[4].y + obj2D.y0, 2, 2, 0, 1);

  //draw wind speed
  float tmp = osd_windSpeed * convert_speed;
  snprintf(tmp_str, sizeof(tmp_str), "%.2f%s", tmp, spd_unit);
  write_string(tmp_str, posX + 15, posY, 0, 0, TEXT_VA_MIDDLE, TEXT_HA_LEFT, 0, SIZE_TO_FONT[0]);
}

void draw_warning(void) {
  bool haswarn = false;
  uint8_t warning[] = { 0, 0, 0,
                        0, 0, 0,
                        0, 0, 0,
                        0 };

  //no GPS fix!
  if (osd_params.Alarm_GPS_status_en == 1 && (osd_fix_type < GPS_OK_FIX_3D)) {
    haswarn = true;
    warning[0] = 1;
  }

  //low batt
  if (osd_params.Alarm_low_batt_en == 1 && (osd_battery_remaining_A < osd_params.Alarm_low_batt)) {
    haswarn = true;
    warning[1] = 1;
  }

  float spd_comparison = osd_groundspeed;
  if (osd_params.Spd_Scale_type == 1) {
    spd_comparison = osd_airspeed;
  }
  spd_comparison *= convert_speed;
  //under speed
  if (osd_params.Alarm_low_speed_en == 1 && (spd_comparison < osd_params.Alarm_low_speed)) {
    haswarn = true;
    warning[2] = 1;
  }

  //over speed
  if (osd_params.Alarm_over_speed_en == 1 && (spd_comparison > osd_params.Alarm_over_speed)) {

    haswarn = true;
    warning[3] = 1;
  }

  float alt_comparison = osd_rel_alt;
  if (osd_params.Alt_Scale_type == 0) {
    alt_comparison = osd_alt;
  }
  //under altitude
  if (osd_params.Alarm_low_alt_en == 1 && (alt_comparison < osd_params.Alarm_low_alt)) {
    haswarn = true;
    warning[4] = 1;
  }

  //over altitude
  if (osd_params.Alarm_over_alt_en == 1 && (alt_comparison > osd_params.Alarm_over_alt)) {
    haswarn = true;
    warning[5] = 1;
  }

  // no home yet
  if (osd_got_home == 0) {
    haswarn = true;
    warning[6] = 1;
  }

  if (osd_params.Alarm_rc_status_en == 1 && rc_lost) {
    haswarn = true;
    warning[7] = 1;
  }

  if (osd_params.Alarm_wfb_status_en == 1 && (wfb_flags & WFB_LINK_LOST)) {
    haswarn = true;
    warning[8] = 1;
  }

  if (osd_params.Alarm_wfb_status_en == 1 && (wfb_flags & WFB_LINK_JAMMED)) {
    haswarn = true;
    warning[9] = 1;
  }

  if (haswarn && (GetSystimeMS() - last_warn_time) >= 1000) {
    last_warn_time = GetSystimeMS();

    do
    {
        last_warn_type = (last_warn_type + 1) % (sizeof(warning) / sizeof(uint8_t));
    }while(!warning[last_warn_type]);

    switch(last_warn_type)
    {
    case 0:
        strncpy(warn_str, "NO GPS FIX", sizeof(warn_str));
        break;

    case 1:
        strncpy(warn_str, "LOW BATTERY", sizeof(warn_str));
        break;

    case 2:
        strncpy(warn_str, "SPEED LOW", sizeof(warn_str));
        break;

    case 3:
        strncpy(warn_str, "OVER SPEED", sizeof(warn_str));
        break;

    case 4:
        strncpy(warn_str, "LOW ALT", sizeof(warn_str));
        break;

    case 5:
        strncpy(warn_str, "HIGH ALT", sizeof(warn_str));
        break;

    case 6:
        strncpy(warn_str, "NO HOME POSITION SET", sizeof(warn_str));
        break;

    case 7:
        strncpy(warn_str, "RC LOST", sizeof(warn_str));
        break;

    case 8:
        strncpy(warn_str, "WFB LOST", sizeof(warn_str));
        break;

    case 9:
        strncpy(warn_str, "WFB JAMMED", sizeof(warn_str));
        break;

    }
  }

  if(!haswarn)
  {
      //Show a new warning immediately
      last_warn_time = 0;
      strcpy(warn_str, "");
  }

  write_string(warn_str, osd_params.Alarm_posX, osd_params.Alarm_posY, 0, 0, TEXT_VA_TOP, osd_params.Alarm_align, 0, SIZE_TO_FONT[osd_params.Alarm_fontsize]);
}

void debug_wps(void) {

//    //debug
//    uint16_t a = 20;
//    for(int i=1; i<wp_counts; i++)
//    {
//        snprintf(tmp_str, sizeof(tmp_str), "WP%d X:%0.12f Y:%0.12f",
//                         (int)wp_list[i].seq, (double)wp_list[i].x,
//                          (double)wp_list[i].y);
//        write_string(tmp_str, 10, a, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, SIZE_TO_FONT[0]);
//        a += 15;
//    }

//    float uav_lat = osd_lat / 10000000.0f;
//    float uav_lon = osd_lon / 10000000.0f;
  float home_lat = osd_home_lat / 10000000.0f;
  float home_lon = osd_home_lon / 10000000.0f;

//    if(osd_fix_type > 1){
//        snprintf(tmp_str, sizeof(tmp_str), "UAV X:%0.12f Y:%0.12f",(double)uav_lat,(double)uav_lon);
//        write_string(tmp_str, 10, a, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, SIZE_TO_FONT[0]);
//        a += 15;
//    }

  if (osd_got_home == 1) {
//        snprintf(tmp_str, sizeof(tmp_str), "home X:%0.12f Y:%0.12f",(double)home_lat,(double)home_lon);
//        write_string(tmp_str, 10, a, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, SIZE_TO_FONT[0]);
//        a += 15;
    snprintf(tmp_str, sizeof(tmp_str), "%0.5f %0.5f", (double)home_lat, (double)home_lon);
    write_string(tmp_str, 70, 210, 0, 0, TEXT_VA_TOP, TEXT_HA_LEFT, 0, SIZE_TO_FONT[0]);
  }

  return;
}

void gen_overlay_rect(float lat, float lon, VECTOR4D_PTR vec) {
  //left:vec.x  top:vec.y  right:vec.z  bottom:vec.w
  if (lon < vec->x) vec->x = lon;
  if (lat > vec->y) vec->y = lat;
  if (lon > vec->z) vec->z = lon;
  if (lat < vec->w) vec->w = lat;
}

VERTEX2DF gps_to_screen_pixel(float lat, float lon, float cent_lat, float cent_lon,
                              float rect_diagonal_half, float cent_x, float cent_y, float radius) {
  float dstlon, dstlat, distance;
  float scaleLongUp, scaleLongDown;
  int dir = 0;

  VERTEX2DF point_ret;

  scaleLongUp   = 1.0f / Fast_Cos(fabs(lat));
  scaleLongDown = Fast_Cos(fabs(lat));

  dstlon = fabs(lon - cent_lon) * 111319.5f * scaleLongDown;
  dstlat = fabs(lat - cent_lat) * 111319.5f;
  distance = sqrt(dstlat * dstlat + dstlon * dstlon);

  dstlon = (lon - cent_lon);
  dstlat = (lat - cent_lat) * scaleLongUp;
  dir = 270 + (atan2(dstlat, -dstlon) * R2D);
  dir = (dir + 360) % 360;
  point_ret.x = cent_x + radius * Fast_Sin(dir) * distance / rect_diagonal_half;
  point_ret.y = cent_y - radius * Fast_Cos(dir) * distance / rect_diagonal_half;

  return point_ret;
}

void draw_flight_mode() {
  if (!enabledAndShownOnPanel(osd_params.FlightMode_en,
                              osd_params.FlightMode_panel)) {
    return;
  }

  char* mode_str = "UNKNOWN";
  switch (autopilot) {
  case 3:       //ardupilotmega
  {
    if (mav_type != 1) {
      if (custom_mode == 0)       mode_str = "STAB";              //manual airframe angle with manual throttle
      else if (custom_mode == 1)  mode_str = "ACRO";              //manual body-frame angular rate with manual throttle
      else if (custom_mode == 2)  mode_str = "ALTH";              //manual airframe angle with automatic throttle
      else if (custom_mode == 3)  mode_str = "AUTO";              //fully automatic waypoint control using mission commands
      else if (custom_mode == 4)  mode_str = "GUID";              //fully automatic fly to coordinate or fly at velocity/direction using GCS immediate commands
      else if (custom_mode == 5)  mode_str = "LOIT";              //automatic horizontal acceleration with automatic throttle
      else if (custom_mode == 6)  mode_str = "RETL";              //automatic return to launching point
      else if (custom_mode == 7)  mode_str = "CIRC";              //automatic circular flight with automatic throttle
      //else if (custom_mode == 8)  mode_str = "POSI"; //Position: auto control
      else if (custom_mode == 9)  mode_str = "LAND";              //automatic landing with horizontal position control
      else if (custom_mode == 10) mode_str = "OFLO";              //deprecated
      else if (custom_mode == 11) mode_str = "DRIF";              //semi-automous position, yaw and throttle control
      else if (custom_mode == 13) mode_str = "SPRT";              //manual earth-frame angular rate control with manual throttle
      else if (custom_mode == 14) mode_str = "FLIP";              //automatically flip the vehicle on the roll axis
      else if (custom_mode == 15) mode_str = "ATUN";              //automatically tune the vehicle's roll and pitch gains
      else if (custom_mode == 16) mode_str = "POSH";              //automatic position hold with manual override, with automatic throttle
      else if (custom_mode == 17) mode_str = "BRAK";              //full-brake using inertial/GPS system, no pilot input
    } else if (mav_type == 1) {          //ArduPlane
      if (custom_mode == 0)       mode_str = "MANU";              //Manual
      else if (custom_mode == 1)  mode_str = "CIRC";              //Circle
      else if (custom_mode == 2)  mode_str = "STAB";              //Stabilize
      else if (custom_mode == 3)  mode_str = "TRNG";              //Training
      else if (custom_mode == 4)  mode_str = "ACRO";              //Acro
      else if (custom_mode == 5)  mode_str = "FBWA";              //Fly_By_Wire_A
      else if (custom_mode == 6)  mode_str = "FBWB";              //Fly_By_Wire_B
      else if (custom_mode == 7)  mode_str = "CRUI";              //Cruise
      else if (custom_mode == 8)  mode_str = "ATUN";              //Auto Tune
      else if (custom_mode == 10) mode_str = "AUTO";              //Auto
      else if (custom_mode == 11) mode_str = "RETL";              //Return to Launch
      else if (custom_mode == 12) mode_str = "LOIT";              //Loiter
      else if (custom_mode == 15) mode_str = "GUID";              //Guided
      else if (custom_mode == 16) mode_str = "INIT";              //Initializing
    }

    break;
  }
  case 12:       //PX4
  {
    union px4_custom_mode custom_mode_px4;
    custom_mode_px4.data = custom_mode;

    if (custom_mode_px4.main_mode == 1) mode_str = "MANUAL";
    else if (custom_mode_px4.main_mode == 2) mode_str = "ALTCTL";
    else if (custom_mode_px4.main_mode == 3) mode_str = "POSCTL";
    else if (custom_mode_px4.main_mode == 4)
    {
      if (custom_mode_px4.sub_mode == 1)  mode_str = "READY";
      else if (custom_mode_px4.sub_mode == 2)  mode_str = "TAKEOFF";
      else if (custom_mode_px4.sub_mode == 3)  mode_str = "LOITER";
      else if (custom_mode_px4.sub_mode == 4)  mode_str = "MISSION";
      else if (custom_mode_px4.sub_mode == 5)  mode_str = "RTL";
      else if (custom_mode_px4.sub_mode == 6)  mode_str = "LAND";
      else if (custom_mode_px4.sub_mode == 7)  mode_str = "RTGS";
      else if (custom_mode_px4.sub_mode == 8)  mode_str = "FOLLOW";
    }
    else if (custom_mode_px4.main_mode == 5) mode_str = "ACRO";
    else if (custom_mode_px4.main_mode == 6) mode_str = "OFFBOARD";
    else if (custom_mode_px4.main_mode == 7) mode_str = "STABILIZE";
    else if (custom_mode_px4.main_mode == 8) mode_str = "RATTITUDE";

    break;
  }

  default:
  {
    mode_str = "----";
  }
  }

  write_string(mode_str, osd_params.FlightMode_posX, osd_params.FlightMode_posY,
               0, 0, TEXT_VA_TOP, osd_params.FlightMode_align, 0,
               SIZE_TO_FONT[osd_params.FlightMode_fontsize]);
}

void draw_arm_state() {
  if (!enabledAndShownOnPanel(osd_params.Arm_en,
                              osd_params.Arm_panel)) {
    return;
  }

  char* tmp_str1 = motor_armed ? "ARMED" : "DISARMED";
  write_string(tmp_str1, osd_params.Arm_posX,
               osd_params.Arm_posY, 0, 0, TEXT_VA_TOP,
               osd_params.Arm_align, 0,
               SIZE_TO_FONT[osd_params.Arm_fontsize]);
}

void draw_battery_voltage() {
  if (!enabledAndShownOnPanel(osd_params.BattVolt_en,
                              osd_params.BattVolt_panel)) {
    return;
  }

  snprintf(tmp_str, sizeof(tmp_str), "%4.1fV", (double) osd_vbat_A);
  write_string(tmp_str, osd_params.BattVolt_posX,
               osd_params.BattVolt_posY, 0, 0, TEXT_VA_TOP,
               osd_params.BattVolt_align, 0,
               SIZE_TO_FONT[osd_params.BattVolt_fontsize]);
}

void draw_battery_current() {
  if (!enabledAndShownOnPanel(osd_params.BattCurrent_en,
                              osd_params.BattCurrent_panel)) {
    return;
  }

  snprintf(tmp_str, sizeof(tmp_str), "%5.1fA", (double) (osd_curr_A * 0.01));
  write_string(tmp_str, osd_params.BattCurrent_posX,
               osd_params.BattCurrent_posY, 0, 0, TEXT_VA_TOP,
               osd_params.BattCurrent_align, 0,
               SIZE_TO_FONT[osd_params.BattCurrent_fontsize]);
}

void draw_battery_remaining() {
  if (!enabledAndShownOnPanel(osd_params.BattRemaining_en,
                              osd_params.BattRemaining_panel)) {
    return;
  }

  snprintf(tmp_str, sizeof(tmp_str), "%3d%%", osd_battery_remaining_A);
  write_string(tmp_str, osd_params.BattRemaining_posX,
               osd_params.BattRemaining_posY, 0, 0, TEXT_VA_TOP,
               osd_params.BattRemaining_align, 0,
               SIZE_TO_FONT[osd_params.BattRemaining_fontsize]);
}

void draw_battery_consumed() {
  if (!enabledAndShownOnPanel(osd_params.BattConsumed_en,
                              osd_params.BattConsumed_panel)) {
    return;
  }

  snprintf(tmp_str, sizeof(tmp_str), "%dmah", (int)osd_curr_consumed_mah);
  write_string(tmp_str, osd_params.BattConsumed_posX,
               osd_params.BattConsumed_posY, 0, 0, TEXT_VA_TOP,
               osd_params.BattConsumed_align, 0,
               SIZE_TO_FONT[osd_params.BattConsumed_fontsize]);
}

void draw_wfb_state() {
  if (!enabledAndShownOnPanel(osd_params.WFBState_en,
                              osd_params.WFBState_panel)) {
    return;
  }

  if (wfb_flags & WFB_LINK_LOST)
  {
      snprintf(tmp_str, sizeof(tmp_str), "WFB LOST");
  }
  else if (wfb_flags & WFB_LINK_JAMMED)
  {
      snprintf(tmp_str, sizeof(tmp_str), "WFB %d/JAM", wfb_rssi);
  }
  else
  {
      snprintf(tmp_str, sizeof(tmp_str), "WFB %d/%d/%d", wfb_rssi, wfb_fec_fixed, wfb_errors);
  }

  write_string(tmp_str,
               osd_params.WFBState_posX,
               osd_params.WFBState_posY, 0, 0, TEXT_VA_TOP,
               osd_params.WFBState_align, 0,
               SIZE_TO_FONT[osd_params.WFBState_fontsize]);
}


void draw_altitude_scale() {
  if (!enabledAndShownOnPanel(osd_params.Alt_Scale_en,
                              osd_params.Alt_Scale_panel)) {
    return;
  }

  uint16_t posX = osd_params.Alt_Scale_posX;
  float alt_shown;

  if (!isnan(osd_bottom_clearance)){
      alt_shown = osd_bottom_clearance;
      snprintf(tmp_str, sizeof(tmp_str), "TALT");
  }else{
      if (osd_params.Alt_Scale_type == 0) {
          alt_shown = osd_alt;
          snprintf(tmp_str, sizeof(tmp_str), "AALT");
      }else{
          alt_shown = osd_rel_alt;
          snprintf(tmp_str, sizeof(tmp_str), "RALT");
      }
  }

  draw_vertical_scale(alt_shown * convert_distance, 40,
                      osd_params.Alt_Scale_align,
                      osd_params.Alt_Scale_posX,
                      osd_params.Alt_Scale_posY, 100, 
		      5, 10, 5, 8, 11,
                      10000, 0);
  if ((osd_params.Alt_Scale_align == 1) && (posX > 15)) {
    posX -= 15;
  }
  write_string(tmp_str, posX,
               osd_params.Alt_Scale_posY - 70, 0, 0, TEXT_VA_TOP,
               osd_params.Alt_Scale_align, 0,
               SIZE_TO_FONT[0]);
  if ((osd_params.Alt_Scale_align == 1) && (posX > 15)) {
    posX += 10;
  }
  write_string("m", posX,
               osd_params.Alt_Scale_posY + 60, 0, 0, TEXT_VA_TOP,
               osd_params.Alt_Scale_align, 0,
               SIZE_TO_FONT[0]);
}

void draw_absolute_altitude() {
  if (!enabledAndShownOnPanel(osd_params.TALT_en,
                              osd_params.TALT_panel)) {
    return;
  }

  float tmp = osd_alt * convert_distance;
  if (tmp < convert_distance_divider) {
    snprintf(tmp_str, sizeof(tmp_str), "AA %d%s", (int) tmp, dist_unit_short);
  }
  else{
    snprintf(tmp_str, sizeof(tmp_str), "AA %0.2f%s", (double) (tmp / convert_distance_divider), dist_unit_long);
  }

  write_string(tmp_str, osd_params.TALT_posX,
               osd_params.TALT_posY, 0, 0, TEXT_VA_TOP,
               osd_params.TALT_align, 0,
               SIZE_TO_FONT[osd_params.TALT_fontsize]);
}

void draw_relative_altitude() {
  if (!enabledAndShownOnPanel(osd_params.Relative_ALT_en,
                              osd_params.Relative_ALT_panel)) {
    return;
  }

  float tmp = osd_rel_alt * convert_distance;
  if (tmp < convert_distance_divider) {
    snprintf(tmp_str, sizeof(tmp_str), "A %d%s", (int) tmp, dist_unit_short);
  }
  else{
    snprintf(tmp_str, sizeof(tmp_str), "A %0.2f%s", (double) (tmp / convert_distance_divider), dist_unit_long);
  }

  write_string(tmp_str, osd_params.Relative_ALT_posX,
               osd_params.Relative_ALT_posY, 0, 0, TEXT_VA_TOP,
               osd_params.Relative_ALT_align, 0,
               SIZE_TO_FONT[osd_params.Relative_ALT_fontsize]);
}

void draw_speed_scale() {
  if (!enabledAndShownOnPanel(osd_params.Speed_scale_en,
                              osd_params.Speed_scale_panel)) {
      return;
  }

  float spd_shown ;

  if (vtol_state == MAV_VTOL_STATE_TRANSITION_TO_FW || vtol_state == MAV_VTOL_STATE_FW || mav_type == MAV_TYPE_FIXED_WING)
  {
      spd_shown = osd_airspeed;
      snprintf(tmp_str, sizeof(tmp_str), "AS");
  } else {
      spd_shown = osd_groundspeed;
      snprintf(tmp_str, sizeof(tmp_str), "GS");
  }

  draw_vertical_scale(spd_shown * convert_speed, 40,
                      osd_params.Speed_scale_align,
                      osd_params.Speed_scale_posX,
                      osd_params.Speed_scale_posY, 100,
		      5, 10, 5, 8, 11,
                      100, 0);
  write_string(tmp_str, osd_params.Speed_scale_posX,
               osd_params.Speed_scale_posY - 70, 0, 0, TEXT_VA_TOP,
               osd_params.Speed_scale_align, 0,
               SIZE_TO_FONT[0]);

  snprintf(tmp_str, sizeof(tmp_str), "%s", spd_unit);
  write_string(tmp_str, osd_params.Speed_scale_posX,
               osd_params.Speed_scale_posY + 60, 0, 0, TEXT_VA_TOP,
               osd_params.Speed_scale_align, 0,
               SIZE_TO_FONT[0]);
}

void draw_ground_speed() {
  if (!enabledAndShownOnPanel(osd_params.TSPD_en,
                              osd_params.TSPD_panel)) {
    return;
  }

  float tmp = osd_groundspeed * convert_speed;
  snprintf(tmp_str, sizeof(tmp_str), "GS: %d %s", (int) tmp, spd_unit);
  write_string(tmp_str, osd_params.TSPD_posX,
               osd_params.TSPD_posY, 0, 0, TEXT_VA_TOP,
               osd_params.TSPD_align, 0,
               SIZE_TO_FONT[osd_params.TSPD_fontsize]);
}

void draw_air_speed() {
  if (!enabledAndShownOnPanel(osd_params.Air_Speed_en,
                              osd_params.Air_Speed_panel)) {
    return;
  }

  float tmp = osd_airspeed * convert_speed;
  snprintf(tmp_str, sizeof(tmp_str), "AS %d%s", (int) tmp, spd_unit);
  write_string(tmp_str, osd_params.Air_Speed_posX,
               osd_params.Air_Speed_posY, 0, 0, TEXT_VA_TOP,
               osd_params.Air_Speed_align, 0,
               SIZE_TO_FONT[osd_params.Air_Speed_fontsize]);
}

void draw_vtol_speed() {
  if (!enabledAndShownOnPanel(osd_params.TSPD_en,
                              osd_params.TSPD_panel)) {
    return;
  }

  float tmp;
  if (vtol_state == MAV_VTOL_STATE_TRANSITION_TO_FW || vtol_state == MAV_VTOL_STATE_FW)
  {
      tmp = osd_airspeed * convert_speed;
      snprintf(tmp_str, sizeof(tmp_str), "AS: %d %s", (int) tmp, spd_unit);
  } else {
      tmp = osd_groundspeed * convert_speed;
      snprintf(tmp_str, sizeof(tmp_str), "GS: %d %s", (int) tmp, spd_unit);
  }

  write_string(tmp_str, osd_params.TSPD_posX,
               osd_params.TSPD_posY, 0, 0, TEXT_VA_TOP,
               osd_params.TSPD_align, 0,
               SIZE_TO_FONT[osd_params.TSPD_fontsize]);
}
