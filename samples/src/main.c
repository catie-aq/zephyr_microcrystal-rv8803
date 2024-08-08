/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>

#include <time.h>
#include <string.h>

/* Wed Dec 31 2025 23:59:55 GMT+0000 */
#define RTC_TEST_GET_SET_TIME	  (1767225595UL)
#define TM_BASE_YEAR 1900
#define TM_OFFSET_MONTH 1

static const struct device *rtc = DEVICE_DT_GET(DT_NODELABEL(rv_8803_c70));

int main(void)
{
	struct rtc_time datetime_set;
	struct rtc_time datetime_get;

	if (!device_is_ready(rtc)) {
		printk("Device is not ready\r\n");
		return 1;
	}

	time_t timer_set = RTC_TEST_GET_SET_TIME;
	gmtime_r(&timer_set, (struct tm *)(&datetime_set));
	memset(&datetime_get, 0xFF, sizeof(datetime_get));
	if (rtc_set_time(rtc, &datetime_set) != 0) {
		printk("Failed to set time\r\n");
	}
	if (rtc_get_time(rtc, &datetime_get) != 0) {
		printk("Failed to get time using rtc_time_get()\r\n");
	}

	printk("RTC_TIME [%02d/%02d/%d %02d:%02d:%02d]\r\n",
			datetime_get.tm_mday,
			datetime_get.tm_mon + TM_OFFSET_MONTH,
			datetime_get.tm_year + TM_BASE_YEAR,
			datetime_get.tm_hour,
			datetime_get.tm_min,
			datetime_get.tm_sec);

	if ((datetime_get.tm_sec < 0) || (datetime_get.tm_sec >= 60)) {
		printk("Invalid tm_sec [%d]\r\n", datetime_get.tm_sec);
	}
	if ((datetime_get.tm_min < 0) || (datetime_get.tm_min >= 60)) {
		printk("Invalid tm_min [%d]\r\n", datetime_get.tm_min);
	}
	if ((datetime_get.tm_hour < 0) || (datetime_get.tm_hour >= 24)) {
		printk("Invalid tm_hour [%d]\r\n", datetime_get.tm_hour);
	}
	if ((datetime_get.tm_mday < 1) || (datetime_get.tm_mday >= 32)) {
		printk("Invalid tm_mday [%d]\r\n", datetime_get.tm_mday);
	}
	if ((datetime_get.tm_year < 125) || (datetime_get.tm_year >= 127)) {
		printk("Invalid tm_year [%d]\r\n", datetime_get.tm_year);
	}
	if ((datetime_get.tm_wday < 1) || (datetime_get.tm_wday >= 7)) {
		printk("Invalid tm_wday [%d]\r\n", datetime_get.tm_wday);
	}

	if ((datetime_get.tm_yday < -1) || (datetime_get.tm_yday >= 366)) {
		printk("Invalid tm_yday [%d]\r\n", datetime_get.tm_yday);
	}
	if (datetime_get.tm_isdst != -1) {
		printk("Invalid tm_idst [%d]\r\n", datetime_get.tm_isdst);
	}
	if ((datetime_get.tm_nsec < 0) || (datetime_get.tm_nsec >= 1000000000)) {
		printk("Invalid tm_nsec [%d]\r\n", datetime_get.tm_nsec);
	}

	while (1) {
		// Example for Real-Time Controller
		if (rtc_get_time(rtc, &datetime_get) != 0) {
			printk("Failed to get time using rtc_time_get()\r\n");
		}
		printk("RTC_TIME [%02d/%02d/%d %02d:%02d:%02d]\r\n",
			datetime_get.tm_mday,
			datetime_get.tm_mon + TM_OFFSET_MONTH,
			datetime_get.tm_year + TM_BASE_YEAR,
			datetime_get.tm_hour,
			datetime_get.tm_min,
			datetime_get.tm_sec);

		k_sleep(K_MSEC(1000));
	}
}
