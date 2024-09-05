/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>

#include <time.h>
#include <string.h>

#define RTC_TEST_GET_SET_TIME (1767225595UL) // Wed Dec 31 2025 23:59:55 GMT+0000
#define TIME_SIZE             64

static const struct device *rtc = DEVICE_DT_GET(DT_NODELABEL(rv88030));

void alarm_callback(const struct device *dev, uint16_t id, void *user_data)
{
	printk("RTC Alarm detected!!\r\n");
}

int main(void)
{
	struct rtc_time datetime_set, datetime_get, datetime_alarm;
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

	datetime_alarm.tm_min = 1;
	datetime_alarm.tm_hour = 0;
	datetime_alarm.tm_mday = 0;
	datetime_alarm.tm_wday = 0;
	if (rtc_alarm_set_time(rtc, 0, RTC_ALARM_TIME_MASK_MINUTE, &datetime_alarm)) {
		printk("Failed to set alarm time using rtc_alarm_set_time()\r\n");
	}
	printk("Setter[%ld] datetime [%d|%d %d:%d]\r\n", RTC_ALARM_TIME_MASK_MINUTE,
	       datetime_alarm.tm_wday, datetime_alarm.tm_mday, datetime_alarm.tm_hour,
	       datetime_alarm.tm_min);

	uint16_t value = 0;
	if (rtc_alarm_get_time(rtc, 0, &value, &datetime_alarm)) {
		printk("Failed to get alarm time using rtc_alarm_get_time()\r\n");
	}
	printk("Getter[%d] datetime [%d|%d %d:%d]\r\n", value, datetime_alarm.tm_wday,
	       datetime_alarm.tm_mday, datetime_alarm.tm_hour, datetime_alarm.tm_min);

	if (rtc_alarm_set_callback(rtc, 0, alarm_callback, NULL)) {
		printk("Failed to set alarm callback using rtc_alarm_set_callback()\r\n");
	}

	while (1) {
		// Example for Real-Time Controller
		if (rtc_get_time(rtc, &datetime_get) != 0) {
			printk("Failed to get time using rtc_time_get()\r\n");
		}

		int value = rtc_alarm_is_pending(rtc, 0);

		strftime(str, TIME_SIZE, "%c", rtc_time_to_tm(&datetime_get));
		printf("RTC_TIME[%d] [%s]\r\n", value, str);

		k_sleep(K_MSEC(1000));
	}
}
