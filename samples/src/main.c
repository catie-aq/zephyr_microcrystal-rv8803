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

#if CONFIG_MFD_RV8803_DETECT_BATTERY_STATE
#include "rv8803_api.h"
#endif /* CONFIG_RV8803_DETECT_BATTERY_STATE */

#define RTC_TEST_GET_SET_TIME         (1767225595UL) // Wed Dec 31 2025 23:59:55 GMT+0000
#define TIME_SIZE                     64
#define RV8803_CLK_FREQUENCY_32768_HZ 0x00
#define RV8803_CLK_FREQUENCY_1024_HZ  0x01
#define RV8803_CLK_FREQUENCY_1_HZ     0x02

#define RV8803_DT_HAS_MFD CONFIG_DT_HAS_MICROCRYSTAL_RV8803_ENABLED
#define RV8803_DT_HAS_RTC CONFIG_DT_HAS_MICROCRYSTAL_RV8803_RTC_ENABLED
#define RV8803_DT_HAS_CNT CONFIG_DT_HAS_MICROCRYSTAL_RV8803_COUNTER_ENABLED
#define RV8803_DT_HAS_CLK CONFIG_DT_HAS_MICROCRYSTAL_RV8803_CLOCK_ENABLED

#define RV8803_NODE     DT_NODELABEL(rv88030)
#define RV8803_RTC_NODE DT_CHILD(RV8803_NODE, rv8803_rtc)
#define RV8803_CNT_NODE DT_CHILD(RV8803_NODE, rv8803_counter)
#define RV8803_CLK_NODE DT_CHILD(RV8803_NODE, rv8803_clock)

#if RV8803_DT_HAS_MFD
static const struct device *rv8803_dev = DEVICE_DT_GET(RV8803_NODE);
#endif
#if RV8803_DT_HAS_RTC
static const struct device *rtc_dev = DEVICE_DT_GET(RV8803_RTC_NODE);
#endif
#if RV8803_DT_HAS_CNT
static const struct device *cnt_dev = DEVICE_DT_GET(RV8803_CNT_NODE);
#endif
#if RV8803_DT_HAS_CLK
static const struct device *clk_dev = DEVICE_DT_GET(RV8803_CLK_NODE);
#endif

#if RV8803_DT_HAS_RTC
#if CONFIG_RTC_ALARM
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
#endif

#if CONFIG_RTC_UPDATE && DT_ENUM_IDX(RV8803_RTC_NODE, update)
void update_callback(const struct device *dev, void *user_data)
{
	printk("RTC Update detected!!\n");
}
#endif
#endif

#if RV8803_DT_HAS_CNT
#if RV8803_DT_HAS_RTC && CONFIG_RTC_UPDATE && DT_ENUM_IDX(RV8803_RTC_NODE, update)
static bool up_flag = false;
#endif
void period_callback(const struct device *dev, void *user_data)
{
	printk("CNT Period detected!!\n");
#if RV8803_DT_HAS_RTC && CONFIG_RTC_UPDATE && DT_ENUM_IDX(RV8803_RTC_NODE, update)
	if (up_flag) {
		up_flag = false;
		if (rtc_update_set_callback(rtc_dev, NULL, NULL)) {
			printk("Failed to set update callback using rtc_update_set_callback()\n");
		}
	} else {
		up_flag = true;
		if (rtc_update_set_callback(rtc_dev, update_callback, NULL)) {
			printk("Failed to set update callback using rtc_update_set_callback()\n");
		}
	}
#endif
}
#endif

int main(void)
{

#if RV8803_DT_HAS_MFD
	if (!device_is_ready(rv8803_dev)) {
		printk("Device is not ready\n");
		return -ENODEV;
	}
	printk("RV8803 device is ready\n");

#if CONFIG_MFD_RV8803_DETECT_BATTERY_STATE
	struct rv8803_battery *battery = (struct rv8803_battery *)rv8803_dev->data;
	printk("RV8803: POR[%d] LOW[%d]\n", battery->power_on_reset, battery->low_battery);
#endif /* CONFIG_MFD_RV8803_DETECT_BATTERY_STATE */
#endif

#if RV8803_DT_HAS_RTC
	struct rtc_time datetime_set, datetime_get;
	char str[TIME_SIZE];
	if (!device_is_ready(rtc_dev)) {
		printk("Device is not ready\n");
		return -ENODEV;
	}
	printk("RTC device is ready\n");
#endif
#if RV8803_DT_HAS_CNT
	struct counter_top_cfg cfg;
	if (!device_is_ready(cnt_dev)) {
		printk("Device is not ready\n");
		return -ENODEV;
	}
	printk("CNT device is ready\n");
#endif
#if RV8803_DT_HAS_CLK
	if (!device_is_ready(clk_dev)) {
		printk("Device is not ready\n");
		return -ENODEV;
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
		return -EINVAL;
	}
	printk("Clock rate[%d]\n", rate);
#endif

#if RV8803_DT_HAS_RTC
	time_t timer_set = RTC_TEST_GET_SET_TIME;
	gmtime_r(&timer_set, (struct tm *)(&datetime_set));
	memset(&datetime_get, 0xFF, sizeof(datetime_get));
	if (rtc_set_time(rtc_dev, &datetime_set) != 0) {
		printk("Failed to set time\n");
		return -EINVAL;
	}
	printk("RTC set time SUCCEED\n");
	if (rtc_get_time(rtc_dev, &datetime_get) != 0) {
		printk("Failed to get time using rtc_time_get()\n");
		return -EINVAL;
	}
	printk("RTC get time SUCCEED\n");

#if CONFIG_RTC_ALARM && DT_PROP(RV8803_RTC_NODE, use_irq)
	/* Alarm setup */
	struct rtc_time datetime_alarm;
	datetime_alarm.tm_min = 1;
	datetime_alarm.tm_hour = 0;
	datetime_alarm.tm_mday = 0;
	datetime_alarm.tm_wday = 0;
	if (rtc_alarm_set_time(rtc_dev, 0, RTC_ALARM_TIME_MASK_MINUTE, &datetime_alarm)) {
		printk("Failed to set alarm time using rtc_alarm_set_time()\n");
		return -EINVAL;
	}
	printk("Setter[%ld] datetime [%d|%d %d:%d]\n", RTC_ALARM_TIME_MASK_MINUTE,
	       datetime_alarm.tm_wday, datetime_alarm.tm_mday, datetime_alarm.tm_hour,
	       datetime_alarm.tm_min);

	uint16_t value = 0;
	if (rtc_alarm_get_time(rtc_dev, 0, &value, &datetime_alarm)) {
		printk("Failed to get alarm time using rtc_alarm_get_time()\n");
		return -EINVAL;
	}
	printk("Getter[%d] datetime [%d|%d %d:%d]\n", value, datetime_alarm.tm_wday,
	       datetime_alarm.tm_mday, datetime_alarm.tm_hour, datetime_alarm.tm_min);
#if DT_ENUM_IDX(RV8803_RTC_NODE, alarm)
	int res = rtc_alarm_set_callback(rtc_dev, 0, alarm_callback, NULL);
	if (res) {
		printk("Failed to set alarm callback using rtc_alarm_set_callback(): [%d]\n", res);
	} else {
		printk("RTC set alarm callback using rtc_alarm_set_callback() SUCCEED\n");
	}
#endif
#endif

#if CONFIG_RTC_UPDATE
	/* Update setup */
#if !RV8803_DT_HAS_CNT && DT_ENUM_IDX(RV8803_RTC_NODE, update)
	if (rtc_update_set_callback(rtc_dev, update_callback, NULL)) {
		printk("Failed to set update callback using rtc_update_set_callback()\n");
	} else {
		printk("RTC set update callback using rtc_update_set_callback() SUCCEED\n");
	}
#endif
#endif
#endif

#if RV8803_DT_HAS_CNT
	/* Counter setup */
	const struct counter_config_info *cnt_config =
		(const struct counter_config_info *)cnt_dev->config;
	printk("Counter: freq[%d]\n", cnt_config->freq);
	printk("Counter: Max_value[%d]\n", cnt_config->max_top_value);
	printk("Counter: 2s ticks[%d]\n", counter_us_to_ticks(cnt_dev, 2000000));

	cfg.ticks = counter_us_to_ticks(cnt_dev, 2000000);
	cfg.callback = period_callback;
	err = counter_set_top_value(cnt_dev, &cfg);
	if (err) {
		printk("Failed to set Counter Top value [%d]\n", err);
	}
	if (counter_start(cnt_dev)) {
		printk("Failed to start Counter\n");
	}
#endif

	while (1) {
#if RV8803_DT_HAS_RTC
		// Example for Real-Time Controller
		if (rtc_get_time(rtc_dev, &datetime_get) != 0) {
			printk("Failed to get time using rtc_time_get()\n");
		}
		char output[32];
		char buf[16];

#if RV8803_DT_HAS_CNT && !DT_PROP(RV8803_CNT_NODE, use_irq)
		int interrupt = counter_get_pending_int(cnt_dev);
		sprintf(buf, "RTC_TIME[%c]", (interrupt) ? '1' : '0');

		if (interrupt) {
			period_callback(cnt_dev, NULL);
		}
#else
		sprintf(buf, "RTC_TIME");
#endif

#if CONFIG_RTC_ALARM && !DT_ENUM_IDX(RV8803_RTC_NODE, alarm)
		int value = rtc_alarm_is_pending(rtc_dev, 0);
		sprintf(output, "%s[%c]", buf, (value) ? '1' : '0');

		if (value) {
			alarm_callback(rtc_dev, 0, NULL);
		}
#else
		sprintf(output, "%s", buf);
#endif

		strftime(str, TIME_SIZE, "%c", rtc_time_to_tm(&datetime_get));
		printf("%s: [%s]\n", output, str);
#endif

		k_sleep(K_MSEC(1000));
	}
}
