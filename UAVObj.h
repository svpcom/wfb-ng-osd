#ifndef __UAVOBJ_H_
#define __UAVOBJ_H_

#include "m2dlib.h"

extern POLYGON2D uav2D;
extern POLYGON2D rollscale2D;
extern POLYGON2D simple_attitude;
extern POLYGON2D home_direction;
extern POLYGON2D home_direction_outline;

void uav2D_init(void);
void simple_attitude_init(void);
void home_direction_init(void);

#endif //__UAVOBJ_H_
