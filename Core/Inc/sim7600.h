/*
 * sim7600.h
 *
 */

#ifndef INC_SIM7600_H_
#define INC_SIM7600_H_

#include <stdint.h>
#include "stm32f4xx_hal.h"

HAL_StatusTypeDef SIM7600_UART_Send(const char *s);
int  SIM7600_ReadLine(char *buf, int max_len, uint32_t timeout_ms);
int  SIM7600_SendAT_WaitFor(const char *cmd, const char *expect, uint32_t timeout_ms);

/* GNSS power */
int  SIM7600_EnableGNSS(void);

/* Raw TCP over AT sockets */
int  SIM7600_TCP_NetOpen(const char *apn);
int  SIM7600_TCP_Open(uint8_t link_id, const char *host, uint16_t port);
int  SIM7600_TCP_Send(uint8_t link_id, const uint8_t *data, int len, uint32_t timeout_ms);
int  SIM7600_TCP_Close(uint8_t link_id);
int  SIM7600_TCP_NetClose(void);

#endif /* INC_SIM7600_H_ */
