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
#include <stdint.h>
#include <assert.h>

#include "UAVObj.h"
#include "osdconfig.h"
#include "graphengine.h"
#include "osdvar.h"


POLYGON2D uav2D;
POLYGON2D rollscale2D;
POLYGON2D simple_attitude;
POLYGON2D home_direction;
POLYGON2D home_direction_outline;


void simple_attitude_init(void) {
  simple_attitude.state     = 1;
  simple_attitude.num_verts = 4;
  simple_attitude.x0        = osd_params.Atti_mp_posX;
  simple_attitude.y0        = osd_params.Atti_mp_posY;
  const int line_length = 60;
  const int line_spacing = 20;
  int x = 0;
  int i = 0;
  for (int index = 0; index < simple_attitude.num_verts; index += 4) {
    x = 12 + i * line_length + i * line_spacing + line_spacing;
    VECTOR2D_INITXYZ(&(simple_attitude.vlist_local[index]), -x, 0);
    VECTOR2D_INITXYZ(&(simple_attitude.vlist_local[index + 1]), -x - line_length, 0);
    VECTOR2D_INITXYZ(&(simple_attitude.vlist_local[index + 2]), x, 0);
    VECTOR2D_INITXYZ(&(simple_attitude.vlist_local[index + 3]), x + line_length, 0);
    i++;
  }

  Scale_Polygon2D(&simple_attitude, atti_mp_scale, atti_mp_scale);
}

void home_direction_init(void) {
  home_direction.state     = 1;
  home_direction.num_verts = 24;
  home_direction.x0        = osd_params.HomeDirection_posX + 10;
  home_direction.y0        = osd_params.HomeDirection_posY + 10;
  VECTOR2D_INITXYZ(&(home_direction.vlist_local[0]), -2, -2);
  VECTOR2D_INITXYZ(&(home_direction.vlist_local[1]), -2, 8);
  VECTOR2D_INITXYZ(&(home_direction.vlist_local[2]), -1, -2);
  VECTOR2D_INITXYZ(&(home_direction.vlist_local[3]), -1, 8);
  VECTOR2D_INITXYZ(&(home_direction.vlist_local[4]), 0, -2);
  VECTOR2D_INITXYZ(&(home_direction.vlist_local[5]), 0, 8);
  VECTOR2D_INITXYZ(&(home_direction.vlist_local[6]), 1, -2);
  VECTOR2D_INITXYZ(&(home_direction.vlist_local[7]), 1, 8);
  VECTOR2D_INITXYZ(&(home_direction.vlist_local[8]), 2, -2);
  VECTOR2D_INITXYZ(&(home_direction.vlist_local[9]), 2, 8);
  VECTOR2D_INITXYZ(&(home_direction.vlist_local[10]), -5, -3);
  VECTOR2D_INITXYZ(&(home_direction.vlist_local[11]), 5, -3);
  VECTOR2D_INITXYZ(&(home_direction.vlist_local[12]), -4, -4);
  VECTOR2D_INITXYZ(&(home_direction.vlist_local[13]), 4, -4);
  VECTOR2D_INITXYZ(&(home_direction.vlist_local[14]), -3, -5);
  VECTOR2D_INITXYZ(&(home_direction.vlist_local[15]), 3, -5);
  VECTOR2D_INITXYZ(&(home_direction.vlist_local[16]), -2, -6);
  VECTOR2D_INITXYZ(&(home_direction.vlist_local[17]), 2, -6);
  VECTOR2D_INITXYZ(&(home_direction.vlist_local[18]), -1, -7);
  VECTOR2D_INITXYZ(&(home_direction.vlist_local[19]), 1, -7);
  VECTOR2D_INITXYZ(&(home_direction.vlist_local[20]), 0, -8);
  VECTOR2D_INITXYZ(&(home_direction.vlist_local[21]), 0, -8);

  home_direction_outline.state     = 1;
  home_direction_outline.num_verts = 14;
  home_direction_outline.x0        = osd_params.HomeDirection_posX;
  home_direction_outline.y0        = osd_params.HomeDirection_posY;
  VECTOR2D_INITXYZ(&(home_direction_outline.vlist_local[0]), -7, -2);
  VECTOR2D_INITXYZ(&(home_direction_outline.vlist_local[1]), 0, -9);
  VECTOR2D_INITXYZ(&(home_direction_outline.vlist_local[2]), 0, -9);
  VECTOR2D_INITXYZ(&(home_direction_outline.vlist_local[3]), 7, -2);
  VECTOR2D_INITXYZ(&(home_direction_outline.vlist_local[4]), 7, -2);
  VECTOR2D_INITXYZ(&(home_direction_outline.vlist_local[5]), 3, -2);
  VECTOR2D_INITXYZ(&(home_direction_outline.vlist_local[6]), -7, -2);
  VECTOR2D_INITXYZ(&(home_direction_outline.vlist_local[7]), -3, -2);
  VECTOR2D_INITXYZ(&(home_direction_outline.vlist_local[8]), 3, -2);
  VECTOR2D_INITXYZ(&(home_direction_outline.vlist_local[9]), 3, 9);
  VECTOR2D_INITXYZ(&(home_direction_outline.vlist_local[10]), -3, -2);
  VECTOR2D_INITXYZ(&(home_direction_outline.vlist_local[11]), -3, 9);
  VECTOR2D_INITXYZ(&(home_direction_outline.vlist_local[12]), -3, 9);
  VECTOR2D_INITXYZ(&(home_direction_outline.vlist_local[13]), 3, 9);
}


void uav2D_init(void) {
  // initialize uav2d
  uav2D.state       = 1;         // turn it on
  uav2D.num_verts   = 38;
  uav2D.x0          = osd_params.Atti_mp_posX;       // position it
  uav2D.y0          = osd_params.Atti_mp_posY;

  int index = 0, i = 0;
  const int lX = 5;
  const int stepY = 11;
  const int hX = 22;
  for (index = 0; index < uav2D.num_verts / 2 - 1; )
  {
    VECTOR2D_INITXYZ(&(uav2D.vlist_local[index]), -lX, stepY * (i + 1));
    VECTOR2D_INITXYZ(&(uav2D.vlist_local[index + 1]), lX, stepY * (i + 1));
    index += 2;
    i++;
  }
  VECTOR2D_INITXYZ(&(uav2D.vlist_local[index]), -hX, 0);
  VECTOR2D_INITXYZ(&(uav2D.vlist_local[index + 1]), hX, 0);
  index += 2;

  i = 0;
  for (; index < uav2D.num_verts; )
  {
    VECTOR2D_INITXYZ(&(uav2D.vlist_local[index]), -lX, -stepY * (i + 1));
    VECTOR2D_INITXYZ(&(uav2D.vlist_local[index + 1]), lX, -stepY * (i + 1));
    index += 2;
    i++;
  }

  // do a quick scale on the vertices

  Scale_Polygon2D(&uav2D, atti_mp_scale, atti_mp_scale);

  // initialize roll scale
  rollscale2D.state       = 1;         // turn it on
  rollscale2D.num_verts   = 13;
  rollscale2D.x0          = osd_params.Atti_mp_posX;       // position it
  rollscale2D.y0          = osd_params.Atti_mp_posY;


  int x, y, theta;
  int mp = (rollscale2D.num_verts - 1) / 2;
  i = mp;
  int radio = 38;
  int arcStep = (mp * 10) / 6;
  for (index = 0; index < mp; index++)
  {
    theta = i * arcStep;
    x = radio * Fast_Sin(theta);
    y = radio * Fast_Cos(theta);
    VECTOR2D_INITXYZ(&(rollscale2D.vlist_local[index]), -x, -y);
    VECTOR2D_INITXYZ(&(rollscale2D.vlist_local[rollscale2D.num_verts - 1 - index]), x, -y);
    i--;
  }

  VECTOR2D_INITXYZ(&(rollscale2D.vlist_local[index]), 0, -radio);
  Scale_Polygon2D(&rollscale2D, atti_mp_scale, atti_mp_scale);
}
