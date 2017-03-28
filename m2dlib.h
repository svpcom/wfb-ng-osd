#ifndef M2DLIB_H__
#define M2DLIB_H__

#include "math3d.h"

// DEFINES ////////////////////////////////////////////////////

#define OBJECT2DV1_MAX_VERTICES           80


// a 2D polygon
typedef struct POLYGON2D_TYP
{
  int state;              // state of polygon
  int num_verts;          // number of vertices
  int x0, y0;             // position of center of polygon
  VERTEX2DF vlist_local[OBJECT2DV1_MAX_VERTICES];       // pointer to vertex list
  VERTEX2DF vlist_trans[OBJECT2DV1_MAX_VERTICES];       // pointer to vertex list

} POLYGON2D, *POLYGON2D_PTR;

void Reset_Polygon2D(POLYGON2D_PTR poly);
int Transform_Polygon2D(POLYGON2D_PTR poly, float roate, float tx, float ty);
int Translate_Polygon2D(POLYGON2D_PTR poly, float dx, float dy);
int Rotate_Polygon2D(POLYGON2D_PTR poly, float theta);
int Scale_Polygon2D(POLYGON2D_PTR poly, float sx, float sy);
int Clip_Line(VECTOR4D_PTR v);

#endif
