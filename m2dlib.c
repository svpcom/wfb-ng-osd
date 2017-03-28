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
 * With Grateful Acknowledgements to the books writen by Andre LaMothe:
 * <Tricks of the 3D Game Programming Gurus-Advanced 3D Graphics and Rasterization>
 * <Tricks of the Windows Game Programming Gurus>
 */
#include "m2dlib.h"
#include "osdvar.h"

//Reset 2d object, just copy local verts to transfer verts
void Reset_Polygon2D(POLYGON2D_PTR poly) {
  for (int curr_vert = 0; curr_vert < poly->num_verts; curr_vert++) {
    VECTOR2D_COPY(&(poly->vlist_trans[curr_vert]),
                  &(poly->vlist_local[curr_vert]));
  }
}

int Transform_Polygon2D(POLYGON2D_PTR poly, float roate, float tx, float ty) {
  // test for valid pointer
  if (!poly)
    return (0);

  // test for negative rotation angle
  if (roate < 0)
    roate += 360;

  // loop and rotate each point, very crude, no lookup!!!
  for (int curr_vert = 0; curr_vert < poly->num_verts; curr_vert++) {
    poly->vlist_trans[curr_vert].x += tx;
    poly->vlist_trans[curr_vert].y += ty;
    // perform rotation
    float xr = (float) poly->vlist_trans[curr_vert].x * Fast_Cos(roate)
               - (float) poly->vlist_trans[curr_vert].y * Fast_Sin(roate);

    float yr = (float) poly->vlist_trans[curr_vert].x * Fast_Sin(roate)
               + (float) poly->vlist_trans[curr_vert].y * Fast_Cos(roate);

    // store result back
    poly->vlist_trans[curr_vert].x = xr;
    poly->vlist_trans[curr_vert].y = yr;

  }   // end for curr_vert

  // return success
  return (1);
}

int Translate_Polygon2D(POLYGON2D_PTR poly, float dx, float dy) {
  // this function translates the center of a polygon

  // test for valid pointer
  if (!poly)
    return (0);

  // translate
  poly->x0 += dx;
  poly->y0 += dy;

  // return success
  return (1);

} // end Translate_Polygon2D

int Rotate_Polygon2D(POLYGON2D_PTR poly, float theta) {
  // this function rotates the local coordinates of the polygon

  // test for valid pointer
  if (!poly)
    return (0);

  // test for negative rotation angle
  if (theta < 0)
    theta += 360;

  // loop and rotate each point, very crude, no lookup!!!
  for (int curr_vert = 0; curr_vert < poly->num_verts; curr_vert++) {
    // perform rotation
    float xr = (float) poly->vlist_trans[curr_vert].x * Fast_Cos(theta)
               - (float) poly->vlist_trans[curr_vert].y * Fast_Sin(theta);

    float yr = (float) poly->vlist_trans[curr_vert].x * Fast_Sin(theta)
               + (float) poly->vlist_trans[curr_vert].y * Fast_Cos(theta);

    // store result back
    poly->vlist_trans[curr_vert].x = xr;
    poly->vlist_trans[curr_vert].y = yr;

  }   // end for curr_vert

  // return success
  return (1);

} // end Rotate_Polygon2D

int Scale_Polygon2D(POLYGON2D_PTR poly, float sx, float sy) {
  // this function scalesthe local coordinates of the polygon

  // test for valid pointer
  if (!poly)
    return (0);

  // loop and scale each point
  for (int curr_vert = 0; curr_vert < poly->num_verts; curr_vert++) {
    // scale and store result back
    poly->vlist_local[curr_vert].x *= sx;
    poly->vlist_local[curr_vert].y *= sy;

  }   // end for curr_vert

  // return success
  return (1);

} // end Scale_Polygon2D

//return 1: visible
//		 0: invisible
int Clip_Line(VECTOR4D_PTR v) {
  // this function clips the sent line using the globally defined clipping
  // region

  // internal clipping codes
#define CLIP_CODE_C  0x0000
#define CLIP_CODE_N  0x0008
#define CLIP_CODE_S  0x0004
#define CLIP_CODE_E  0x0002
#define CLIP_CODE_W  0x0001

#define CLIP_CODE_NE 0x000a
#define CLIP_CODE_SE 0x0006
#define CLIP_CODE_NW 0x0009
#define CLIP_CODE_SW 0x0005

  int x1 = v->x, y1 = v->y, x2 = v->z, y2 = v->w;

  int xc1 = x1, yc1 = y1, xc2 = x2, yc2 = y2;

  int p1_code = 0, p2_code = 0;

  // determine codes for p1 and p2
  if (y1 < atti_3d_min_clipY)
    p1_code |= CLIP_CODE_N;
  else if (y1 > atti_3d_max_clipY)
    p1_code |= CLIP_CODE_S;

  if (x1 < atti_3d_min_clipX)
    p1_code |= CLIP_CODE_W;
  else if (x1 > atti_3d_max_clipX)
    p1_code |= CLIP_CODE_E;

  if (y2 < atti_3d_min_clipY)
    p2_code |= CLIP_CODE_N;
  else if (y2 > atti_3d_max_clipY)
    p2_code |= CLIP_CODE_S;

  if (x2 < atti_3d_min_clipX)
    p2_code |= CLIP_CODE_W;
  else if (x2 > atti_3d_max_clipX)
    p2_code |= CLIP_CODE_E;

  // try and trivially reject
  if ((p1_code & p2_code))
    return (0);

  // test for totally visible, if so leave points untouched
  if (p1_code == 0 && p2_code == 0)
    return (1);

  // determine end clip point for p1
  switch (p1_code) {
  case CLIP_CODE_C:
    break;

  case CLIP_CODE_N: {
    yc1 = atti_3d_min_clipY;
    xc1 = x1 + 0.5 + (atti_3d_min_clipY - y1) * (x2 - x1) / (y2 - y1);
  }
  break;
  case CLIP_CODE_S: {
    yc1 = atti_3d_max_clipY;
    xc1 = x1 + 0.5 + (atti_3d_max_clipY - y1) * (x2 - x1) / (y2 - y1);
  }
  break;

  case CLIP_CODE_W: {
    xc1 = atti_3d_min_clipX;
    yc1 = y1 + 0.5 + (atti_3d_min_clipX - x1) * (y2 - y1) / (x2 - x1);
  }
  break;

  case CLIP_CODE_E: {
    xc1 = atti_3d_max_clipX;
    yc1 = y1 + 0.5 + (atti_3d_max_clipX - x1) * (y2 - y1) / (x2 - x1);
  }
  break;

  // these cases are more complex, must compute 2 intersections
  case CLIP_CODE_NE: {
    // north hline intersection
    yc1 = atti_3d_min_clipY;
    xc1 = x1 + 0.5 + (atti_3d_min_clipY - y1) * (x2 - x1) / (y2 - y1);

    // test if intersection is valid, of so then done, else compute next
    if (xc1 < atti_3d_min_clipX || xc1 > atti_3d_max_clipX) {
      // east vline intersection
      xc1 = atti_3d_max_clipX;
      yc1 = y1 + 0.5 + (atti_3d_max_clipX - x1) * (y2 - y1) / (x2 - x1);
    }     // end if

  }
  break;

  case CLIP_CODE_SE: {
    // south hline intersection
    yc1 = atti_3d_max_clipY;
    xc1 = x1 + 0.5 + (atti_3d_max_clipY - y1) * (x2 - x1) / (y2 - y1);

    // test if intersection is valid, of so then done, else compute next
    if (xc1 < atti_3d_min_clipX || xc1 > atti_3d_max_clipX) {
      // east vline intersection
      xc1 = atti_3d_max_clipX;
      yc1 = y1 + 0.5 + (atti_3d_max_clipX - x1) * (y2 - y1) / (x2 - x1);
    }     // end if

  }
  break;

  case CLIP_CODE_NW: {
    // north hline intersection
    yc1 = atti_3d_min_clipY;
    xc1 = x1 + 0.5 + (atti_3d_min_clipY - y1) * (x2 - x1) / (y2 - y1);

    // test if intersection is valid, of so then done, else compute next
    if (xc1 < atti_3d_min_clipX || xc1 > atti_3d_max_clipX) {
      xc1 = atti_3d_min_clipX;
      yc1 = y1 + 0.5 + (atti_3d_min_clipX - x1) * (y2 - y1) / (x2 - x1);
    }     // end if

  }
  break;

  case CLIP_CODE_SW: {
    // south hline intersection
    yc1 = atti_3d_max_clipY;
    xc1 = x1 + 0.5 + (atti_3d_max_clipY - y1) * (x2 - x1) / (y2 - y1);

    // test if intersection is valid, of so then done, else compute next
    if (xc1 < atti_3d_min_clipX || xc1 > atti_3d_max_clipX) {
      xc1 = atti_3d_min_clipX;
      yc1 = y1 + 0.5 + (atti_3d_min_clipX - x1) * (y2 - y1) / (x2 - x1);
    }     // end if

  }
  break;

  default:
    break;

  }   // end switch

  // determine clip point for p2
  switch (p2_code) {
  case CLIP_CODE_C:
    break;

  case CLIP_CODE_N: {
    yc2 = atti_3d_min_clipY;
    xc2 = x2 + (atti_3d_min_clipY - y2) * (x1 - x2) / (y1 - y2);
  }
  break;

  case CLIP_CODE_S: {
    yc2 = atti_3d_max_clipY;
    xc2 = x2 + (atti_3d_max_clipY - y2) * (x1 - x2) / (y1 - y2);
  }
  break;

  case CLIP_CODE_W: {
    xc2 = atti_3d_min_clipX;
    yc2 = y2 + (atti_3d_min_clipX - x2) * (y1 - y2) / (x1 - x2);
  }
  break;

  case CLIP_CODE_E: {
    xc2 = atti_3d_max_clipX;
    yc2 = y2 + (atti_3d_max_clipX - x2) * (y1 - y2) / (x1 - x2);
  }
  break;

  // these cases are more complex, must compute 2 intersections
  case CLIP_CODE_NE: {
    // north hline intersection
    yc2 = atti_3d_min_clipY;
    xc2 = x2 + 0.5 + (atti_3d_min_clipY - y2) * (x1 - x2) / (y1 - y2);

    // test if intersection is valid, of so then done, else compute next
    if (xc2 < atti_3d_min_clipX || xc2 > atti_3d_max_clipX) {
      // east vline intersection
      xc2 = atti_3d_max_clipX;
      yc2 = y2 + 0.5 + (atti_3d_max_clipX - x2) * (y1 - y2) / (x1 - x2);
    }     // end if

  }
  break;

  case CLIP_CODE_SE: {
    // south hline intersection
    yc2 = atti_3d_max_clipY;
    xc2 = x2 + 0.5 + (atti_3d_max_clipY - y2) * (x1 - x2) / (y1 - y2);

    // test if intersection is valid, of so then done, else compute next
    if (xc2 < atti_3d_min_clipX || xc2 > atti_3d_max_clipX) {
      // east vline intersection
      xc2 = atti_3d_max_clipX;
      yc2 = y2 + 0.5 + (atti_3d_max_clipX - x2) * (y1 - y2) / (x1 - x2);
    }     // end if

  }
  break;

  case CLIP_CODE_NW: {
    // north hline intersection
    yc2 = atti_3d_min_clipY;
    xc2 = x2 + 0.5 + (atti_3d_min_clipY - y2) * (x1 - x2) / (y1 - y2);

    // test if intersection is valid, of so then done, else compute next
    if (xc2 < atti_3d_min_clipX || xc2 > atti_3d_max_clipX) {
      xc2 = atti_3d_min_clipX;
      yc2 = y2 + 0.5 + (atti_3d_min_clipX - x2) * (y1 - y2) / (x1 - x2);
    }     // end if

  }
  break;

  case CLIP_CODE_SW: {
    // south hline intersection
    yc2 = atti_3d_max_clipY;
    xc2 = x2 + 0.5 + (atti_3d_max_clipY - y2) * (x1 - x2) / (y1 - y2);

    // test if intersection is valid, of so then done, else compute next
    if (xc2 < atti_3d_min_clipX || xc2 > atti_3d_max_clipX) {
      xc2 = atti_3d_min_clipX;
      yc2 = y2 + 0.5 + (atti_3d_min_clipX - x2) * (y1 - y2) / (x1 - x2);
    }     // end if

  }
  break;

  default:
    break;

  }   // end switch

  // do bounds check
  if ((xc1 < atti_3d_min_clipX) || (xc1 > atti_3d_max_clipX)
      || (yc1 < atti_3d_min_clipY) || (yc1 > atti_3d_max_clipY)
      || (xc2 < atti_3d_min_clipX) || (xc2 > atti_3d_max_clipX)
      || (yc2 < atti_3d_min_clipY) || (yc2 > atti_3d_max_clipY)) {
    return (0);
  }   // end if

  // store vars back
  v->x = xc1;
  v->y = yc1;
  v->z = xc2;
  v->w = yc2;

  return (1);

} // end Clip_Line

