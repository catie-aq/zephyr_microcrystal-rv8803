/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>

#include <time.h>
#include <string.h>

#define RTC_TEST_GET_SET_TIME	  (1767225595UL) // Wed Dec 31 2025 23:59:55 GMT+0000
#define TIME_SIZE 64

static const struct device *rtc = DEVICE_DT_GET(DT_NODELABEL(rv_8803_c70));

int main(void)
{
	struct rtc_time datetime_set;
	struct rtc_time datetime_get;
	char str[TIME_SIZE];

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

	while (1) {
		// Example for Real-Time Controller
		if (rtc_get_time(rtc, &datetime_get) != 0) {
			printk("Failed to get time using rtc_time_get()\r\n");
		}

		strftime(str, TIME_SIZE, "%c", rtc_time_to_tm(&datetime_get));
		printk("RTC_TIME [%s]\r\n", str);

		k_sleep(K_MSEC(1000));
	}
}
