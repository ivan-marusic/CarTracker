/*
 * debug.h
 *
 */

#ifndef INC_DEBUG_H_
#define INC_DEBUG_H_

#include "stdio.h"
#include "stdint.h"
#include "main.h"

#define LOG_INFO(fmt, ...) printf("[INFO] " fmt "\r\n", ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) printf("[WARN] " fmt "\r\n", ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) printf("[ERROR] " fmt "\r\n", ##__VA_ARGS__)
#define FEED_WATCHDOG() HAL_IWDG_Refresh(&hiwdg)

#endif /* INC_DEBUG_H_ */
