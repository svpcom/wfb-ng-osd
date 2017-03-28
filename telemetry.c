#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <stdio.h>
#include <stdlib.h>
#include "telemetry.h"

void telemetry_init(telemetry_data_t *td) {
	td->voltage = 0;
	td->ampere = 0;
	td->altitude = 0;
	td->longitude = 0;
	td->latitude = 0;
	td->heading = 0;
	td->speed = 0;
        td->climb = 0;
        td->throttle = 0;
	td->x = 0;
	td->y = 0;
	td->z = 0;
	td->ew = 0;
	td->ns = 0;
	td->roll = 0;
	td->pitch = 0;
	td->rssi = 0;
	td->airspeed = 0;
	td->sats = 0;
	td->fix = 0;
}
