#pragma once

#include <stdint.h>
#include <time.h>

typedef struct {
    float voltage;
    float ampere;
    float baro_altitude;
    float altitude;
    float climb;
    uint16_t throttle;
    double longitude;
    double latitude;
    float heading;
    float speed;
    int16_t x, y, z;
    int16_t ew, ns;
    int16_t roll, pitch;
    uint8_t rssi;
    uint8_t airspeed;
    uint8_t sats;
    uint8_t fix;
} telemetry_data_t;


void telemetry_init(telemetry_data_t *td);
