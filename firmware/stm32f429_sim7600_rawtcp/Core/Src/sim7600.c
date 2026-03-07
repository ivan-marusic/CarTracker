/*
 * sim7600.c
 *
 */
#include "sim7600.h"
#include "string.h"
#include "stdio.h"
#include "debug.h"
#include "stm32f4xx_hal_uart.h"

#include <math.h>
#include <ctype.h>

extern UART_HandleTypeDef huart2;

#define STEP_DELAY() HAL_Delay(80)
#define FLY_HOST "cartracker-proxy.fly.dev"
#define FLY_PORT 80


HAL_StatusTypeDef SIM7600_UART_Send(const char *s)
{
    HAL_StatusTypeDef st = HAL_UART_Transmit(&huart2, (uint8_t*)s, strlen(s), HAL_MAX_DELAY);
    if (st != HAL_OK) LOG_WARN("SIM7600: UART send error %d", st);
    return st;
}

int SIM7600_ReadLine(char *buf, int max_len, uint32_t timeout_ms)
{
    int idx = 0;
    uint32_t t0 = HAL_GetTick();

    while (HAL_GetTick() - t0 < timeout_ms) {
        uint8_t c;
        if (HAL_UART_Receive(&huart2, &c, 1, 1000) == HAL_OK) {
            if (idx < max_len - 1) buf[idx++] = (char)c;
            if (c == '\n') {
                buf[idx] = '\0';
                return idx;
            }
        }
    }

    return -1;
}

int SIM7600_SendAT_WaitFor(const char *cmd, const char *expect, uint32_t timeout_ms)
{
    LOG_INFO("SIM7600: AT cmd: %s (expect: %s)", cmd, expect);
    SIM7600_UART_Send(cmd);

    char buf[512]; int n = 0;
    uint32_t t0 = HAL_GetTick();

    while (HAL_GetTick() - t0 < timeout_ms) {
        uint8_t c;
        if (HAL_UART_Receive(&huart2, &c, 1, 50) == HAL_OK) {
            if (n < (int)sizeof(buf) - 1) buf[n++] = (char)c;
            buf[n] = '\0';

            if (strstr(buf, expect)) return 0;
            if (strstr(buf, "ERROR")) return -1;
        }
    }
    LOG_WARN("SIM7600: Timeout waiting for '%s', rx='%s'", expect, buf);
    return -2;
}

/* ----------------- RAW TCP over modem ----------------- */

// Send raw bytes (not zero-terminated) over UART
static HAL_StatusTypeDef SIM7600_UART_SendBuf(const uint8_t *buf, uint16_t len)
{
    return HAL_UART_Transmit(&huart2, (uint8_t*)buf, len, HAL_MAX_DELAY);
}

static int SIM7600_WaitForChar(uint8_t want, uint32_t timeout_ms)
{
    uint32_t t0 = HAL_GetTick();
    while (HAL_GetTick() - t0 < timeout_ms) {
        uint8_t c;
        if (HAL_UART_Receive(&huart2, &c, 1, 20) == HAL_OK) {
            if (c == want) return 0;
        }
    }
    return -1; // timeout
}

int SIM7600_TCP_NetOpen(const char *apn)
{
    // Basic init
    SIM7600_SendAT_WaitFor("AT\r", "OK", 2000);
    HAL_Delay(80);
    SIM7600_SendAT_WaitFor("ATE0\r", "OK", 2000);
    HAL_Delay(80);
    SIM7600_SendAT_WaitFor("AT+CFUN=1\r", "OK", 3000);
    HAL_Delay(120);

    // Attach + APN
    SIM7600_SendAT_WaitFor("AT+CGATT=1\r", "OK", 10000);
    HAL_Delay(80);

    char cmd[96];
    snprintf(cmd, sizeof(cmd), "AT+CGDCONT=1,\"IP\",\"%s\"\r", apn ? apn : "onomondo");
    SIM7600_SendAT_WaitFor(cmd, "OK", 3000);
    HAL_Delay(80);

    SIM7600_SendAT_WaitFor("AT+CSOCKSETPN=1\r", "OK", 2000);
    HAL_Delay(50);

    SIM7600_SendAT_WaitFor("AT+NETOPEN?\r", "OK", 2000);

    uint32_t t0 = HAL_GetTick();
    int opened = 0;

    if (SIM7600_UART_Send("AT+NETOPEN\r") != HAL_OK) return -1;

    char buf[256]; int n = 0;
    while (HAL_GetTick() - t0 < 25000) {
        uint8_t c;
        if (HAL_UART_Receive(&huart2, &c, 1, 200) == HAL_OK) {
            if (n < (int)sizeof(buf) - 1) buf[n++] = (char)c;
            buf[n] = '\0';
            if (strstr(buf, "+NETOPEN: 0")) { opened = 1; break; }
            if (strstr(buf, "ERROR")) break;
        }
    }

    int saw_ip = (SIM7600_SendAT_WaitFor("AT+IPADDR\r", "OK", 3000) == 0);

    opened = opened || saw_ip;

    SIM7600_SendAT_WaitFor("AT+CIPMODE=0\r", "OK", 2000);

    LOG_INFO("NETOPEN OK");
    return opened ? 0 : -1;
}


int SIM7600_TCP_OpenAndSend(uint8_t link_id,
                            const char *host, uint16_t port,
                            const uint8_t *payload, int len,
                            uint32_t send_timeout_ms)
{
    if (!host || !payload || len <= 0) return -1;

    char cmd[160];
    snprintf(cmd, sizeof(cmd), "AT+CIPOPEN=%u,\"TCP\",\"%s\",%u\r", link_id, host, port);
    LOG_INFO("SIM7600: CIPOPEN cmd: %s", cmd);

    {
        uint8_t c; uint32_t t0 = HAL_GetTick();
        while (HAL_GetTick() - t0 < 50) {
            if (HAL_UART_Receive(&huart2, &c, 1, 5) != HAL_OK) break;
        }
    }

    if (SIM7600_UART_Send(cmd) != HAL_OK) return -10;

    char buf[512]; int n = 0;
    uint32_t t0 = HAL_GetTick();
    int opened = 0;

    while (HAL_GetTick() - t0 < 30000) {
        uint8_t c;
        if (HAL_UART_Receive(&huart2, &c, 1, 200) == HAL_OK) {
            if (n < (int)sizeof(buf) - 1) buf[n++] = (char)c;
            buf[n] = '\0';

            char *p = strstr(buf, "+CIPOPEN:");
            if (p) {
                int id = -1, code = -999;
                if (sscanf(p, "+CIPOPEN: %d,%d", &id, &code) == 2 && id == (int)link_id) {
                    if (code == 0) { opened = 1; break; }
                    LOG_ERROR("SIM7600: CIPOPEN failed code=%d", code);
                    return -20 - code;
                }
            }
            if (strstr(buf, "+IPCLOSE:")) {
                LOG_ERROR("SIM7600: peer closed right after open");
                return -30;
            }
            if (strstr(buf, "ERROR")) {
                LOG_ERROR("SIM7600: CIPOPEN immediate ERROR");
                return -11;
            }
        }
    }
    if (!opened) {
        LOG_WARN("SIM7600: CIPOPEN timeout. RX='%s'", buf);
        return -15;
    }

    char send_cmd[48];
    snprintf(send_cmd, sizeof(send_cmd), "AT+CIPSEND=%u,%d\r", link_id, len);
    if (SIM7600_UART_Send(send_cmd) != HAL_OK) return -31;

    if (SIM7600_WaitForChar('>', 2000) != 0) {
        LOG_ERROR("SIM7600: No '>' prompt for CIPSEND");
        return -32;
    }

    if (SIM7600_UART_SendBuf(payload, (uint16_t)len) != HAL_OK) {
        LOG_ERROR("SIM7600: UART send payload failed");
        return -33;
    }

    char rbuf[512]; int rn = 0;
    uint32_t t_send = HAL_GetTick();
    while (HAL_GetTick() - t_send < send_timeout_ms) {
        uint8_t c;
        if (HAL_UART_Receive(&huart2, &c, 1, 200) == HAL_OK) {
            if (rn < (int)sizeof(rbuf) - 1) rbuf[rn++] = (char)c;
            rbuf[rn] = '\0';

            if (strstr(rbuf, "SEND OK")) {
                LOG_INFO("SIM7600: SEND OK");
                break;
            }
            if (strstr(rbuf, "SEND FAIL") || strstr(rbuf, "ERROR") || strstr(rbuf, "FAIL")) {
                LOG_ERROR("SIM7600: Send failed, rbuf='%s'", rbuf);
                return -34;
            }
            if (strstr(rbuf, "HTTP/1.1")) {
                LOG_INFO("SIM7600: HTTP response detected (treat as success)");
                break;
            }
            if (strstr(rbuf, "+IPCLOSE:")) {
                LOG_INFO("SIM7600: Peer closed after send; rbuf='%s'", rbuf);
                break;
            }
        }
    }

    return 0;
}

int SIM7600_HTTP_Post_Fly(const char *json_body)
{
    if (!json_body) return -1;
    const int json_len = (int)strlen(json_body);

    char req[1024];
    int n = snprintf(req, sizeof(req),
        "POST /update HTTP/1.1\r\n"
        "Host: cartracker-proxy.fly.dev\r\n"
        "Content-Type: application/json\r\n"
        "Connection: close\r\n"
        "Content-Length: %d\r\n"
        "\r\n"
        "%s",
        json_len, json_body);

    if (n <= 0 || n >= (int)sizeof(req)) {
        LOG_ERROR("HTTP req too large or snprintf error (n=%d)", n);
        return -2;
    }

    int r = SIM7600_TCP_OpenAndSend(0, FLY_HOST, FLY_PORT,
                                    (const uint8_t*)req, n, 35000);
    if (r != 0) {
        LOG_ERROR("OpenAndSend failed (%d)", r);
        SIM7600_TCP_Close(0);
        return r;
    }

    char line[256];
    uint32_t t0 = HAL_GetTick();
    while (HAL_GetTick() - t0 < 2000) {
        int l = SIM7600_ReadLine(line, sizeof(line), 300);
        if (l > 0) LOG_INFO("HTTP RX: %s", line);
    }

    SIM7600_TCP_Close(0);
    return 0;
}


int SIM7600_ResetAndWaitReady(uint32_t timeout_ms)
{
    // Hard reset
    SIM7600_SendAT_WaitFor("AT+CFUN=1,1\r", "OK", 2000);

    uint32_t t0 = HAL_GetTick();
    char line[256];
    int got_rdy = 0, got_cpin = 0;

    while (HAL_GetTick() - t0 < timeout_ms) {
        int l = SIM7600_ReadLine(line, sizeof(line), 1500);
        if (l <= 0) continue;

        LOG_INFO("BOOT: %s", line);

        if (strstr(line, "RDY")) got_rdy = 1;
        if (strstr(line, "+CPIN: READY")) got_cpin = 1;

        // Minimalno: RDY + CPIN READY
        if (got_rdy && got_cpin) {

			HAL_Delay(500);
            return 0;
        }
    }
    return -1; // timeout
}

int SIM7600_TCP_Open(uint8_t link_id, const char *host, uint16_t port)
{
    char cmd[160];
    snprintf(cmd, sizeof(cmd), "AT+CIPOPEN=%u,\"TCP\",\"%s\",%u\r", link_id, host, port);
    LOG_INFO("SIM7600: CIPOPEN cmd: %s", cmd);

    {
        uint8_t c; uint32_t t0 = HAL_GetTick();
        while (HAL_GetTick() - t0 < 50) {
            if (HAL_UART_Receive(&huart2, &c, 1, 5) != HAL_OK) break;
        }
    }

    if (SIM7600_UART_Send(cmd) != HAL_OK) {
        LOG_ERROR("SIM7600: UART send failed for CIPOPEN");
        return -10;
    }

    char buf[512]; int n = 0;
    uint32_t t0 = HAL_GetTick();

    while (HAL_GetTick() - t0 < 30000)
    {
        uint8_t c;
        if (HAL_UART_Receive(&huart2, &c, 1, 200) == HAL_OK) {
            if (n < (int)sizeof(buf) - 1) buf[n++] = (char)c;
            buf[n] = '\0';


            char *p = strstr(buf, "+CIPOPEN:");
            if (p) {
                int id = -1, code = -999;
                if (sscanf(p, "+CIPOPEN: %d,%d", &id, &code) == 2) {
                    if (id == (int)link_id) {
                        if (code == 0) {
                            LOG_INFO("SIM7600: CIPOPEN OK (id=%d)", link_id);
                            return 0;
                        } else {
                            LOG_ERROR("SIM7600: CIPOPEN failed, code=%d", code);
                            return -20 - code;
                        }
                    }
                }
            }

            if (strstr(buf, "+IPCLOSE:")) {
                LOG_ERROR("SIM7600: peer closed immediately");
                return -30;
            }

            if (strstr(buf, "ERROR")) {
                LOG_ERROR("SIM7600: CIPOPEN immediate ERROR");
                return -11;
            }
        }
    }

    LOG_WARN("SIM7600: CIPOPEN URC timeout. RX='%s'", buf);
    return -15;
}

int SIM7600_TCP_Send(uint8_t link_id, const uint8_t *data, int len, uint32_t timeout_ms)
{
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%u,%d\r", link_id, len);

    if (SIM7600_UART_Send(cmd) != HAL_OK) return -1;

    if (SIM7600_WaitForChar('>', 2000) != 0) {
        LOG_ERROR("SIM7600: No '>' prompt for CIPSEND");
        return -2;
    }


    if (SIM7600_UART_SendBuf(data, (uint16_t)len) != HAL_OK) {
        LOG_ERROR("SIM7600: UART send payload failed");
    	return -3;
    }

    char line[256];
    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < timeout_ms) {
        int l = SIM7600_ReadLine(line, sizeof(line), 500);
        if (l <= 0) continue;
        if (strstr(line, "SEND OK")) return 0;
        if (strstr(line, "SEND FAIL") || strstr(line, "ERROR") || strstr(line, "FAIL"))
        {
        	LOG_ERROR("SIM7600: Send failed, line=%s", line);
        	return -4;
        }
    }
    LOG_WARN("SIM7600: Timeout waiting for SEND OK");
    return -5;
}

int SIM7600_TCP_Close(uint8_t link_id)
{
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "AT+CIPCLOSE=%u\r", link_id);
    SIM7600_SendAT_WaitFor(cmd, "OK", 5000);
    return 0;
}

int SIM7600_TCP_NetClose(void)
{
    SIM7600_SendAT_WaitFor("AT+NETCLOSE\r", "OK", 10000);
    return 0;
}

static int ddmm_to_decimal(const char *ddmm, int is_lon, double *out_deg)
{
    if (!ddmm || !out_deg || !isdigit((unsigned char)ddmm[0])) return -1;

    double val = atof(ddmm);
    if (val <= 0.0) return -2;

    if (is_lon) {
        int deg = (int)(val / 100.0);
        double mins = val - (deg * 100.0);
        *out_deg = deg + mins / 60.0;
    } else {
        int deg = (int)(val / 100.0);
        double mins = val - (deg * 100.0);
        *out_deg = deg + mins / 60.0;
    }
    return 0;
}

static int make_iso8601_utc(const char *ddmmyy, const char *hhmmss, char *out, int out_sz)
{
    if (!ddmmyy || !hhmmss || !out || out_sz < 21) return -1;
    if (strlen(ddmmyy) < 6 || strlen(hhmmss) < 6)  return -2;

    int dd = (ddmmyy[0]-'0')*10 + (ddmmyy[1]-'0');
    int mm = (ddmmyy[2]-'0')*10 + (ddmmyy[3]-'0');
    int yy = (ddmmyy[4]-'0')*10 + (ddmmyy[5]-'0');
    int year = 2000 + yy; // SIM7600 daje 2-znamenk. godinu

    int HH = (hhmmss[0]-'0')*10 + (hhmmss[1]-'0');
    int MM = (hhmmss[2]-'0')*10 + (hhmmss[3]-'0');
    int SS = (hhmmss[4]-'0')*10 + (hhmmss[5]-'0');

    snprintf(out, out_sz, "%04d-%02d-%02dT%02d:%02d:%02dZ", year, mm, dd, HH, MM, SS);
    return 0;
}


int send_gps_minimal_to_fly(void)
{
    char line[256];

    SIM7600_UART_Send("AT+CGPSINFO\r");

    int len = SIM7600_ReadLine(line, sizeof(line), 2000);
    if (len <= 0 || strstr(line, "+CGPSINFO") == NULL) {
        LOG_WARN("No CGPSINFO line");
        return -1;
    }
    LOG_INFO("RAW CGPSINFO: %s", line);

    char lat_s[24]={0}, ns=0, lon_s[24]={0}, ew=0, date_s[16]={0}, time_s[24]={0};
    int matched = sscanf(line,
        "+CGPSINFO: %23[^,],%c,%23[^,],%c,%15[^,],%23[^,]",
        lat_s, &ns, lon_s, &ew, date_s, time_s);

    if (matched < 6) {
        LOG_WARN("CGPSINFO incomplete (matched=%d)", matched);
        return -2;
    }
    if (strlen(lat_s)==0 || strlen(lon_s)==0) {
        LOG_WARN("No fix yet (empty lat/lon)");
        return -2;
    }

    double lat_deg = 0.0, lon_deg = 0.0;
    if (ddmm_to_decimal(lat_s, 0, &lat_deg) != 0 ||
        ddmm_to_decimal(lon_s, 1, &lon_deg) != 0) {
        LOG_WARN("Failed to parse lat/lon ddmm");
        return -3;
    }
    if (ns == 'S') lat_deg = -lat_deg;
    if (ew == 'W') lon_deg = -lon_deg;

    char iso[32];
    if (make_iso8601_utc(date_s, time_s, iso, sizeof(iso)) != 0) {
        strcpy(iso, "1970-01-01T00:00:00Z");
    }

    char json[160];
    int n = snprintf(json, sizeof(json),
        "{\"latitude\":%.6f,\"longitude\":%.6f,\"timestamp\":\"%s\"}",
        lat_deg, lon_deg, iso);

    if (n <= 0 || n >= (int)sizeof(json)) {
        LOG_ERROR("JSON build error (n=%d)", n);
        return -4;
    }

    int r = SIM7600_HTTP_Post_Fly(json);
    if (r != 0) {
        LOG_ERROR("HTTP POST FAIL (%d)", r);
        return -5;
    }

    LOG_INFO("Minimal GPS sent OK: %s", json);
    return 0;
}
