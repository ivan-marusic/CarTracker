/*
 * gnss.h
 *
 */

#ifndef INC_GNSS_H_
#define INC_GNSS_H_

#include <stdint.h>

typedef struct {
    float  latitude;
    float  longitude;
    char   iso8601[32]; // "YYYY-MM-DDTHH:MM:SSZ"
} GNSS_Location;

int GNSS_FormatTimestampISO(const char *raw, char *out, uint32_t out_sz);
int SIM7600_ReadLocation_GNSS(GNSS_Location *loc);
int SIM7600_GetLocation_GNSS(GNSS_Location *loc, uint32_t timeout_ms);
static int send_telemetry_once(uint8_t link_id, const char *device_id);

#endif /* INC_GNSS_H_ */
