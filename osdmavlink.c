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

#include "osdmavlink.h"
#include "osdvar.h"
#include "osdconfig.h"
#include "osdrender.h"

float Rad2Deg(float x)
{
  return x * (180.0F / M_PI);
}

void parse_mavlink_packet(uint8_t *buf, int buflen)
{
    mavlink_status_t status;
    mavlink_message_t msg;
    uint8_t mavtype;

    for(int i = 0; i < buflen; i++)
    {
        uint8_t c = buf[i];
        if (mavlink_parse_char(0, c, &msg, &status))
        {
            //handle msg
            switch (msg.msgid)
            {
            case MAVLINK_MSG_ID_HEARTBEAT:
            {
                if ((msg.compid != 1) && (msg.compid != 50)) {
                    // MAVMSG not from ardupilot(component ID:1) or pixhawk(component ID:50)
                    break;
                }

                mavtype = mavlink_msg_heartbeat_get_type(&msg);
                if (mavtype == 6) {
                    // MAVMSG from GCS
                    break;
                }

                mav_system    = msg.sysid;
                mav_component = msg.compid;
                mav_type      = mavtype;
                autopilot = mavlink_msg_heartbeat_get_autopilot(&msg);
                base_mode = mavlink_msg_heartbeat_get_base_mode(&msg);
                custom_mode = mavlink_msg_heartbeat_get_custom_mode(&msg);

                last_motor_armed = motor_armed;
                motor_armed = base_mode & (1 << 7);

                if (!last_motor_armed && motor_armed) {
                    armed_start_time = GetSystimeMS();
                }

                if (last_motor_armed && !motor_armed) {
                    total_armed_time = GetSystimeMS() - armed_start_time + total_armed_time;
                    armed_start_time = 0;
                }
            }
            break;

            case MAVLINK_MSG_ID_HOME_POSITION:
            {
                osd_home_lat = mavlink_msg_home_position_get_latitude(&msg) / 1e7;
                osd_home_lon = mavlink_msg_home_position_get_longitude(&msg) / 1e7;
                osd_home_alt = mavlink_msg_home_position_get_altitude(&msg) / 1000;
                osd_got_home = 1;
                break;
            }

            case MAVLINK_MSG_ID_EXTENDED_SYS_STATE:
            {
                vtol_state = mavlink_msg_extended_sys_state_get_vtol_state(&msg);
                break;
            }

            case MAVLINK_MSG_ID_SYS_STATUS:
            {
                osd_vbat_A = (mavlink_msg_sys_status_get_voltage_battery(&msg) / 1000.0f);                 //Battery voltage, in millivolts (1 = 1 millivolt)
                osd_curr_A = mavlink_msg_sys_status_get_current_battery(&msg);                 //Battery current, in 10*milliamperes (1 = 10 milliampere)
                osd_battery_remaining_A = mavlink_msg_sys_status_get_battery_remaining(&msg);                 //Remaining battery energy: (0%: 0, 100%: 100)
                //custom_mode = mav_component;//Debug
                //osd_nav_mode = mav_system;//Debug
            }
            break;

            case MAVLINK_MSG_ID_BATTERY_STATUS:
            {
                osd_curr_consumed_mah = mavlink_msg_battery_status_get_current_consumed(&msg);
            }
            break;

            case MAVLINK_MSG_ID_GPS_RAW_INT:
            {
                osd_lat = mavlink_msg_gps_raw_int_get_lat(&msg) / 10000000.0;
                osd_lon = mavlink_msg_gps_raw_int_get_lon(&msg) / 10000000.0;
                osd_fix_type = mavlink_msg_gps_raw_int_get_fix_type(&msg);
                osd_hdop = mavlink_msg_gps_raw_int_get_eph(&msg);
                osd_satellites_visible = mavlink_msg_gps_raw_int_get_satellites_visible(&msg);
            }
            break;

            case MAVLINK_MSG_ID_GPS2_RAW:
            {
                osd_lat2 = mavlink_msg_gps2_raw_get_lat(&msg) / 10000000.0;
                osd_lon2 = mavlink_msg_gps2_raw_get_lon(&msg) / 10000000.0;
                osd_fix_type2 = mavlink_msg_gps2_raw_get_fix_type(&msg);
                osd_hdop2 = mavlink_msg_gps2_raw_get_eph(&msg);
                osd_satellites_visible2 = mavlink_msg_gps2_raw_get_satellites_visible(&msg);
            }
            break;

            case MAVLINK_MSG_ID_VFR_HUD:
            {
                osd_airspeed = mavlink_msg_vfr_hud_get_airspeed(&msg);
                osd_groundspeed = mavlink_msg_vfr_hud_get_groundspeed(&msg);
                osd_heading = mavlink_msg_vfr_hud_get_heading(&msg);                 // 0..360 deg, 0=north
                osd_throttle = mavlink_msg_vfr_hud_get_throttle(&msg);
                osd_alt = mavlink_msg_vfr_hud_get_alt(&msg);
                osd_climb = mavlink_msg_vfr_hud_get_climb(&msg);
            }
            break;

            case MAVLINK_MSG_ID_ALTITUDE:
            {
                osd_bottom_clearance = mavlink_msg_altitude_get_bottom_clearance(&msg);
                osd_rel_alt = mavlink_msg_altitude_get_altitude_relative(&msg);
            }
            break;

            case MAVLINK_MSG_ID_ATTITUDE:
            {
                osd_pitch = Rad2Deg(mavlink_msg_attitude_get_pitch(&msg));
                osd_roll = Rad2Deg(mavlink_msg_attitude_get_roll(&msg));
                osd_yaw = Rad2Deg(mavlink_msg_attitude_get_yaw(&msg));
            }
            break;

            case MAVLINK_MSG_ID_NAV_CONTROLLER_OUTPUT:
            {
                nav_roll = mavlink_msg_nav_controller_output_get_nav_roll(&msg);
                nav_pitch = mavlink_msg_nav_controller_output_get_nav_pitch(&msg);
                nav_bearing = mavlink_msg_nav_controller_output_get_nav_bearing(&msg);
                wp_target_bearing = mavlink_msg_nav_controller_output_get_target_bearing(&msg);
                wp_dist = mavlink_msg_nav_controller_output_get_wp_dist(&msg);
                alt_error = mavlink_msg_nav_controller_output_get_alt_error(&msg);
                aspd_error = mavlink_msg_nav_controller_output_get_aspd_error(&msg);
                xtrack_error = mavlink_msg_nav_controller_output_get_xtrack_error(&msg);
            }
            break;

            case MAVLINK_MSG_ID_MISSION_CURRENT:
            {
                wp_number = (uint8_t)mavlink_msg_mission_current_get_seq(&msg);
            }
            break;

            case MAVLINK_MSG_ID_RC_CHANNELS_RAW:
            {
                if (!osd_chan_cnt_above_eight)
                {
                    osd_chan1_raw = mavlink_msg_rc_channels_raw_get_chan1_raw(&msg);
                    osd_chan2_raw = mavlink_msg_rc_channels_raw_get_chan2_raw(&msg);
                    osd_chan3_raw = mavlink_msg_rc_channels_raw_get_chan3_raw(&msg);
                    osd_chan4_raw = mavlink_msg_rc_channels_raw_get_chan4_raw(&msg);
                    osd_chan5_raw = mavlink_msg_rc_channels_raw_get_chan5_raw(&msg);
                    osd_chan6_raw = mavlink_msg_rc_channels_raw_get_chan6_raw(&msg);
                    osd_chan7_raw = mavlink_msg_rc_channels_raw_get_chan7_raw(&msg);
                    osd_chan8_raw = mavlink_msg_rc_channels_raw_get_chan8_raw(&msg);
                    osd_rssi = mavlink_msg_rc_channels_raw_get_rssi(&msg);
                }
            }
            break;

            case MAVLINK_MSG_ID_RC_CHANNELS:
            {
                osd_chan_cnt_above_eight = true;
                osd_chan1_raw = mavlink_msg_rc_channels_get_chan1_raw(&msg);
                osd_chan2_raw = mavlink_msg_rc_channels_get_chan2_raw(&msg);
                osd_chan3_raw = mavlink_msg_rc_channels_get_chan3_raw(&msg);
                osd_chan4_raw = mavlink_msg_rc_channels_get_chan4_raw(&msg);
                osd_chan5_raw = mavlink_msg_rc_channels_get_chan5_raw(&msg);
                osd_chan6_raw = mavlink_msg_rc_channels_get_chan6_raw(&msg);
                osd_chan7_raw = mavlink_msg_rc_channels_get_chan7_raw(&msg);
                osd_chan8_raw = mavlink_msg_rc_channels_get_chan8_raw(&msg);
                osd_chan9_raw = mavlink_msg_rc_channels_get_chan9_raw(&msg);
                osd_chan10_raw = mavlink_msg_rc_channels_get_chan10_raw(&msg);
                osd_chan11_raw = mavlink_msg_rc_channels_get_chan11_raw(&msg);
                osd_chan12_raw = mavlink_msg_rc_channels_get_chan12_raw(&msg);
                osd_chan13_raw = mavlink_msg_rc_channels_get_chan13_raw(&msg);
                osd_chan14_raw = mavlink_msg_rc_channels_get_chan14_raw(&msg);
                osd_chan15_raw = mavlink_msg_rc_channels_get_chan15_raw(&msg);
                osd_chan16_raw = mavlink_msg_rc_channels_get_chan16_raw(&msg);
                osd_rssi = mavlink_msg_rc_channels_get_rssi(&msg);
            }
            break;

            case MAVLINK_MSG_ID_RADIO_STATUS:
            {
                if ((msg.sysid != 3) || (msg.compid != 68)) {
                    break;
                }

                wfb_rssi = (int8_t)mavlink_msg_radio_status_get_rssi(&msg);
                wfb_errors = mavlink_msg_radio_status_get_rxerrors(&msg);
                wfb_fec_fixed = mavlink_msg_radio_status_get_fixed(&msg);
                wfb_flags = mavlink_msg_radio_status_get_remnoise(&msg);
            }
            break;

            case MAVLINK_MSG_ID_STATUSTEXT:
            {
                osd_message_queue_tail = (osd_message_queue_tail + 1) % OSD_MAX_MESSAGES;
                osd_message_t *item = osd_message_queue + osd_message_queue_tail;
                item->severity = mavlink_msg_statustext_get_severity(&msg);
                mavlink_msg_statustext_get_text(&msg, item->message);
                item->message[sizeof(item->message) - 1] = '\0';
                printf("Message: %s\n", item->message);
            }
            break;


            default:
                //Do nothing
                break;
            }   //end switch(msg.msgid)
        }
    }
}

