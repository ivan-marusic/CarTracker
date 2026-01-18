/*
 * gnss.c
 *
 */

#include "gnss.h"
#include "sim7600.h"
#include "string.h"
#include "stdlib.h"
#include "stdio.h"
#include "debug.h"

static int send_telemetry_once(uint8_t link_id, const char *device_id)
{
    GNSS_Location loc;
    int ret = SIM7600_ReadLocation_GNSS(&loc); // Assumes GNSS is already enabled
    if (ret != 0) {
        LOG_WARN("No GNSS fix yet (%d)", ret);
        return ret;
    }

    // Build a compact JSON line (end with '\n' so server can line-parse)
    char json[192];
    int n = snprintf(json, sizeof(json),
        "{\"id\":\"%s\",\"ts\":\"%s\",\"lat\":%.6f,\"lon\":%.6f}\n",
        device_id, loc.iso8601, loc.latitude, loc.longitude);
    if (n <= 0) return -1;

    return SIM7600_TCP_Send(link_id, (const uint8_t*)json, n, 8000);
}


int GNSS_FormatTimestampISO(const char *raw, char *out, uint32_t out_sz)
{
    if (!raw || strlen(raw) < 14 || out_sz < 21)
    {
        LOG_WARN("GNSS: Timestamp raw string invalid: '%s'", raw ? raw : "(null)");
        return -1;
    }

    char year[5] = {0}, mon[3] = {0}, day[3] = {0};
    char hour[3] = {0}, min[3] = {0}, sec[3] = {0};

    memcpy(year, raw, 4);
    memcpy(mon,  raw + 4, 2);
    memcpy(day,  raw + 6, 2);
    memcpy(hour, raw + 8, 2);
    memcpy(min,  raw + 10, 2);
    memcpy(sec,  raw + 12, 2);

    snprintf(out, out_sz, "%s-%s-%sT%s:%s:%sZ",
             year, mon, day, hour, min, sec);
    LOG_INFO("GNSS: Formatted timestamp: %s", out);
    return 0;
}

int SIM7600_ReadLocation_GNSS(GNSS_Location *loc)
{
    if (!loc) return -1;

    LOG_INFO("GNSS: Sending AT+CGNSINF");
    SIM7600_SendAT_WaitFor("AT+CGNSINF\r", "+CGNSINF:", 1000);

    char line[256];
    uint32_t start = HAL_GetTick();
    int got_line = 0;

    LOG_INFO("GNSS: Waiting for +CGNSINF response");
    while (HAL_GetTick() - start < 1000)
    {
        int len = SIM7600_ReadLine(line, sizeof(line), 200);
        if (len <= 0) continue;

        LOG_INFO("GNSS: RX line: %s", line);

        if (strstr(line, "+CGNSINF:"))
        {
            got_line = 1;
            break;
        }
    }

    if (!got_line)
    {
        LOG_WARN("GNSS: No +CGNSINF line received");
        return -2;
    }

    char *p = strstr(line, "+CGNSINF:");
    if (!p)
    {
        LOG_ERROR("GNSS: +CGNSINF line malformed");
        return -3;
    }

    char *saveptr;
    int   field_idx = 0;
    int   fix = 0;
    char  utc_raw[32] = {0};
    double dlat = 0.0, dlon = 0.0;

    p += strlen("+CGNSINF:");

    for (char *tok = strtok_r(p, ",", &saveptr);
         tok != NULL;
         tok = strtok_r(NULL, ",", &saveptr), field_idx++)
    {
        switch (field_idx)
        {
            case 0: /* run status */ break;
            case 1: fix = atoi(tok); break;
            case 2: strncpy(utc_raw, tok, sizeof(utc_raw)-1); break;
            case 3: dlat = atof(tok); break;
            case 4: dlon = atof(tok); break;
            default: break;
        }
    }

    LOG_INFO("GNSS: fix=%d, raw_time=%s, lat=%f, lon=%f",
             fix, utc_raw, dlat, dlon);

    if (fix == 0)
    {
        LOG_WARN("GNSS: No fix yet");
        return -4;
    }

    loc->latitude  = (float)dlat;
    loc->longitude = (float)dlon;
    loc->iso8601[0] = '\0';

    GNSS_FormatTimestampISO(utc_raw, loc->iso8601, sizeof(loc->iso8601));

    LOG_INFO("GNSS: Fix OK lat=%.6f lon=%.6f time=%s",
             loc->latitude, loc->longitude, loc->iso8601);

    return 0;
}

int SIM7600_GetLocation_GNSS(GNSS_Location *loc, uint32_t timeout_ms)
{
    if (!loc) return -1;

    LOG_INFO("GNSS: Enabling GNSS and waiting for fix, timeout=%lu ms", timeout_ms);
    SIM7600_EnableGNSS();

    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < timeout_ms)
    {
        int ret = SIM7600_ReadLocation_GNSS(loc);
        if (ret == 0)
        {
            LOG_INFO("GNSS: Fix acquired");
            return 0;
        }

        LOG_WARN("GNSS: ReadLocation returned %d, retrying...", ret);
        HAL_Delay(1000);
    }

    LOG_ERROR("GNSS: Timeout waiting for fix");
    return -2;
}

