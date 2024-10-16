/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/counter.h>
#include <zephyr/drivers/clock_control.h>

#include <time.h>
#include <string.h>

#if CONFIG_RV8803_BATTERY_ENABLE
#include "rv8803.h"
#endif /* CONFIG_RV8803_BATTERY_ENABLE */

#define RTC_TEST_GET_SET_TIME         (1767225595UL) // Wed Dec 31 2025 23:59:55 GMT+0000
#define TIME_SIZE                     64
#define RV8803_CLK_FREQUENCY_32768_HZ 0x00
#define RV8803_CLK_FREQUENCY_1024_HZ  0x01
#define RV8803_CLK_FREQUENCY_1_HZ     0x02

#define RV8803_NODE     DT_NODELABEL(rv88030)
#define RV8803_RTC_NODE DT_CHILD(RV8803_NODE, rv8803_rtc)
#define RV8803_CNT_NODE DT_CHILD(RV8803_NODE, rv8803_cnt)
#define RV8803_CLK_NODE DT_CHILD(RV8803_NODE, rv8803_clk)

static const struct device *rv8803_dev = DEVICE_DT_GET(RV8803_NODE);
static const struct device *rtc_dev = DEVICE_DT_GET(RV8803_RTC_NODE);
static const struct device *cnt_dev = DEVICE_DT_GET(RV8803_CNT_NODE);
static const struct device *clk_dev = DEVICE_DT_GET(RV8803_CLK_NODE);

static bool freq_32k = true;

void alarm_callback(const struct device *dev, uint16_t id, void *user_data)
{
	if (freq_32k) {
		printk("RTC Alarm detected: set rate[1024 Hz]!!\n");
		if (clock_control_set_rate(clk_dev, NULL, (void *)RV8803_CLK_FREQUENCY_1024_HZ) !=
		    0) {
			printk("Failed to set clock rate\n");
		}
		freq_32k = false;
	} else {
		printk("RTC Alarm detected: set rate[32768 Hz]!!\n");
		if (clock_control_set_rate(clk_dev, NULL, (void *)RV8803_CLK_FREQUENCY_32768_HZ) !=
		    0) {
			printk("Failed to set clock rate\n");
		}
		freq_32k = true;
	}
}

void update_callback(const struct device *dev, void *user_data)
{
	printk("RTC Update detected!!\n");
}

void period_callback(const struct device *dev, void *user_data)
{
	printk("CNT Period detected!!\n");
}

int main(void)
{
	struct rtc_time datetime_set, datetime_get, datetime_alarm;
	struct counter_top_cfg cfg;
	char str[TIME_SIZE];

	if (!device_is_ready(rv8803_dev)) {
		printk("Device is not ready\n");
		return 1;
	}
	printk("RV8803 device is ready\n");

#if CONFIG_RV8803_BATTERY_ENABLE
	struct rv8803_data *data = rv8803_dev->data;
	printk("RV8803: POR[%d] LOW[%d]\n", data->bat->power_on_reset, data->bat->low_battery);
#endif /* CONFIG_RV8803_BATTERY_ENABLE */

	if (!device_is_ready(rtc_dev)) {
		printk("Device is not ready\n");
		return 1;
	}
	printk("RTC device is ready\n");
	if (!device_is_ready(cnt_dev)) {
		printk("Device is not ready\n");
		return 1;
	}
	printk("CNT device is ready\n");
	if (!device_is_ready(clk_dev)) {
		printk("Device is not ready\n");
		return 1;
	}
	printk("CLK device is ready\n");

	int err = clock_control_set_rate(clk_dev, NULL, (void *)RV8803_CLK_FREQUENCY_32768_HZ);
	if (err == -EALREADY) {
		printk("Clock rate already set\n");
	} else if (err != 0) {
		printk("Failed to set clock rate[%d]\n", err);
	}

	uint32_t rate;
	if (clock_control_get_rate(clk_dev, NULL, &rate) != 0) {
		printk("Failed to get clock rate\n");
	}
	printk("Clock rate[%d]\n", rate);

	time_t timer_set = RTC_TEST_GET_SET_TIME;
	gmtime_r(&timer_set, (struct tm *)(&datetime_set));
	memset(&datetime_get, 0xFF, sizeof(datetime_get));
	if (rtc_set_time(rtc_dev, &datetime_set) != 0) {
		printk("Failed to set time\n");
	}
	printk("RTC set time succeed\n");
	if (rtc_get_time(rtc_dev, &datetime_get) != 0) {
		printk("Failed to get time using rtc_time_get()\n");
	}
	printk("RTC get time succeed\n");

	/* Alarm setup */
	datetime_alarm.tm_min = 1;
	datetime_alarm.tm_hour = 0;
	datetime_alarm.tm_mday = 0;
	datetime_alarm.tm_wday = 0;
	if (rtc_alarm_set_time(rtc_dev, 0, RTC_ALARM_TIME_MASK_MINUTE, &datetime_alarm)) {
		printk("Failed to set alarm time using rtc_alarm_set_time()\n");
	}
	printk("Setter[%ld] datetime [%d|%d %d:%d]\n", RTC_ALARM_TIME_MASK_MINUTE,
	       datetime_alarm.tm_wday, datetime_alarm.tm_mday, datetime_alarm.tm_hour,
	       datetime_alarm.tm_min);

	uint16_t value = 0;
	if (rtc_alarm_get_time(rtc_dev, 0, &value, &datetime_alarm)) {
		printk("Failed to get alarm time using rtc_alarm_get_time()\n");
	}
	printk("Getter[%d] datetime [%d|%d %d:%d]\n", value, datetime_alarm.tm_wday,
	       datetime_alarm.tm_mday, datetime_alarm.tm_hour, datetime_alarm.tm_min);

	if (rtc_alarm_set_callback(rtc_dev, 0, alarm_callback, NULL)) {
		printk("Failed to set alarm callback using rtc_alarm_set_callback()\n");
	}

	/* Update setup */
	if (rtc_update_set_callback(rtc_dev, update_callback, NULL)) {
		printk("Failed to set update callback using rtc_update_set_callback()\n");
	}

	/* Counter setup */
	const struct counter_config_info *cnt_config =
		(const struct counter_config_info *)cnt_dev->config;
	printk("Counter: freq[%d]\n", cnt_config->freq);
	printk("Counter: Max_value[%d]\n", cnt_config->max_top_value);
	printk("Counter: 2s ticks[%d]\n", counter_us_to_ticks(cnt_dev, 2000000));

	cfg.ticks = counter_us_to_ticks(cnt_dev, 2000000);
	cfg.callback = period_callback;
	if (counter_set_top_value(cnt_dev, &cfg)) {
		printk("Failed to set Counter Top value\n");
	}
	if (counter_start(cnt_dev)) {
		printk("Failed to start Counter\n");
	}

	while (1) {
		// Example for Real-Time Controller
		if (rtc_get_time(rtc_dev, &datetime_get) != 0) {
			printk("Failed to get time using rtc_time_get()\n");
		}

		int value = rtc_alarm_is_pending(rtc_dev, 0);

		strftime(str, TIME_SIZE, "%c", rtc_time_to_tm(&datetime_get));
		printf("RTC_TIME[%d] [%s]\n", value, str);

		k_sleep(K_MSEC(1000));
	}
}
