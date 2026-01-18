/*
 * sim7600.c
 *
 */

#include "sim7600.h"
#include "string.h"
#include "stdio.h"
#include "debug.h"
#include "stm32f4xx_hal_uart.h"
#include "gnss.h"

extern UART_HandleTypeDef huart2;

/* ----------------- UART + AT helpers ----------------- */

HAL_StatusTypeDef SIM7600_UART_Send(const char *s)
{
    HAL_StatusTypeDef st = HAL_UART_Transmit(&huart2, (uint8_t*)s, strlen(s), HAL_MAX_DELAY);
    if (st != HAL_OK) LOG_WARN("SIM7600: UART send error %d", st);
    return st;
}

int SIM7600_ReadLine(char *buf, int max_len, uint32_t timeout_ms)
{
    int idx = 0;
    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < timeout_ms) {
        uint8_t c;
        if (HAL_UART_Receive(&huart2, &c, 1, 50) == HAL_OK) {
            if (idx < max_len - 1) buf[idx++] = c;
            if (c == '\n') break;
        }
    }
    if (idx == 0) return -1;
    buf[idx] = '\0';
    return idx;
}

int SIM7600_SendAT_WaitFor(const char *cmd, const char *expect, uint32_t timeout_ms)
{
    LOG_INFO("SIM7600: AT cmd: %s (expect: %s)", cmd, expect);
    SIM7600_UART_Send(cmd);
    char line[256];
    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < timeout_ms) {
        int len = SIM7600_ReadLine(line, sizeof(line), 200);
        if (len <= 0) continue;
        LOG_INFO("SIM7600: RX: %s", line);
        if (strstr(line, expect)) return 0;
        if (strstr(line, "ERROR")) { LOG_ERROR("SIM7600: ERROR in response"); return -1; }
    }
    LOG_WARN("SIM7600: Timeout waiting for '%s'", expect);
    return -2;
}

/* ----------------- GNSS ----------------- */

int SIM7600_EnableGNSS(void)
{
    LOG_INFO("SIM7600: Enabling GNSS");
    return SIM7600_SendAT_WaitFor("AT+CGNSPWR=1\r", "OK", 2000);
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

	SIM7600_SendAT_WaitFor("AT\r", "OK", 1000);
    SIM7600_SendAT_WaitFor("ATE0\r", "OK", 1000);			// echo off
    SIM7600_SendAT_WaitFor("AT+CFUN=1\r", "OK", 3000);
    SIM7600_SendAT_WaitFor("AT+CNMP=38\r", "OK", 3000);
    SIM7600_SendAT_WaitFor("AT+NETCLOSE\r", "OK", 2000); 	// best-effort

    // Attach and set APN
    SIM7600_SendAT_WaitFor("AT+CGATT=1\r", "OK", 5000);

    char cmd[96];
    snprintf(cmd, sizeof(cmd), "AT+CGDCONT=1,\"IP\",\"%s\"\r", apn ? apn : "TM");
    SIM7600_SendAT_WaitFor(cmd, "OK", 2000);


    // Open the modem's internal IP stack
    // Expectation: "+NETOPEN: 0" on success (some FW returns OK then URC)
    int ret = SIM7600_SendAT_WaitFor("AT+NETOPEN\r", "+NETOPEN: 0", 15000);
    if (ret != 0) { LOG_ERROR("NETOPEN failed (%d)", ret); return -1; }


    // Optional: manual receive mode to control pulling data
    SIM7600_SendAT_WaitFor("AT+CIPRXGET=1\r", "OK", 2000);	// 1=manual, 0=URC push
    // Non-transparent mode
    SIM7600_SendAT_WaitFor("AT+CIPMODE=0\r", "OK", 2000);

    LOG_INFO("SIM7600: NETOPEN OK");
    return 0;
}

int SIM7600_TCP_Open(uint8_t link_id, const char *host, uint16_t port)
{
    char cmd[160];
    snprintf(cmd, sizeof(cmd), "AT+CIPOPEN=%u,\"TCP\",\"%s\",%u\r", link_id, host, port);

    // Expect "+CIPOPEN: <id>,0" on success
    int ret = SIM7600_SendAT_WaitFor(cmd, "+CIPOPEN:", 15000);
    if (ret != 0) {
    	LOG_ERROR("SIM7600: CIPOPEN failed (%d)", ret);
        return -1;
    }

    // We should verify "<id>,0" specifically
    char line[256];
    uint32_t start = HAL_GetTick();
    while (HAL_GetTick() - start < 2500) {
        int len = SIM7600_ReadLine(line, sizeof(line), 200);
        if (len <= 0) continue;
        int id, code;
        if (sscanf(line, "+CIPOPEN: %d,%d", &id, &code) == 2 && id == link_id && code == 0)
            return 0;
        if (strstr(line, "ERROR") || strstr(line, "FAIL")) break;
    }
    return -2;
}

int SIM7600_TCP_Send(uint8_t link_id, const uint8_t *data, int len, uint32_t timeout_ms)
{
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "AT+CIPSEND=%u,%d\r", link_id, len);

    if (SIM7600_UART_Send(cmd) != HAL_OK) return -1;

    // Wait for '>' prompt
    if (SIM7600_WaitForChar('>', 2000) != 0) {
    	LOG_ERROR("SIM7600: No '>' prompt for CIPSEND");
    	return -2;
    }

    // Send payload
    if (SIM7600_UART_SendBuf(data, (uint16_t)len) != HAL_OK) {
        LOG_ERROR("SIM7600: UART send payload failed");
    	return -3;
    }

    // Now wait for "SEND OK" (or "ERROR")
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

/* ----------------- Telemetry ----------------- */

static int send_location_once(uint8_t link_id)
{
    GNSS_Location loc;
    int ret = SIM7600_ReadLocation_GNSS(&loc);
    if (ret != 0) { LOG_WARN("GNSS not ready (%d)", ret); return ret; }

    char json[192];
    int n = snprintf(json, sizeof(json),
        "{\"latitude\":%.6f,\"longitude\":%.6f,\"timestamp\":\"%s\"}\n",
        loc.latitude, loc.longitude, loc.iso8601);
    if (n <= 0) return -1;

    return SIM7600_TCP_Send(link_id, (const uint8_t*)json, n, 8000);
}

