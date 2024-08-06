/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microcrystal_rv_8083_c7

#include "rv_8803_c7.h"

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>


LOG_MODULE_REGISTER(rv_8803_c7, CONFIG_RTC_LOG_LEVEL);

struct rv_8803_c7_data {
};

static int rv_8803_c7_set_time(const struct device *dev, const struct rtc_time *timeptr) {
	// Valid date are between 2000 and 2099
	if ((timeptr == NULL) || (timeptr->tm_year < RV8803C7_CORRECT_YEAR_LEAP_MIN) || (timeptr->tm_year > RV8803C7_CORRECT_YEAR_LEAP_MAX)) {
		LOG_ERR("invalid time");
		return -EINVAL;
	}

	LOG_DBG("Set time: year[%u] month[%u] mday[%u] wday[%u] hours[%u] minutes[%u] seconds[%u]",
			timeptr->tm_year,
			timeptr->tm_mon,
			timeptr->tm_mday,
			timeptr->tm_wday,
			timeptr->tm_hour,
			timeptr->tm_min,
			timeptr->tm_sec);
	
	// Init variables for i2c communication
	const struct rv_8803_c7_config *config = dev->config;
	uint8_t regs[8];

	regs[0] = RV8803C7_REGISTER_SECONDS;
	regs[1] = bin2bcd(timeptr->tm_sec) & RV8803C7_SECONDS_BITS;
	regs[2] = bin2bcd(timeptr->tm_min) & RV8803C7_MINUTES_BITS;
	regs[3] = bin2bcd(timeptr->tm_hour) & RV8803C7_HOURS_BITS;
	regs[4] = bin2bcd(timeptr->tm_wday) & RV8803C7_WEEKDAY_BITS;
	regs[5] = bin2bcd(timeptr->tm_mday) & RV8803C7_DATE_BITS;
	regs[6] = bin2bcd(timeptr->tm_mon) & RV8803C7_MONTH_BITS;
	regs[7] = bin2bcd(timeptr->tm_year - RV8803C7_CORRECT_YEAR_LEAP_MIN) & RV8803C7_YEAR_BITS;

	return i2c_write_dt(&config->i2c_bus, regs, 8);
}

static int rv_8803_c7_get_time(const struct device *dev, struct rtc_time *timeptr) {
	if (timeptr == NULL) {
		return -EINVAL;
	}

	const struct rv_8803_c7_config *config = dev->config;
	uint8_t regs[7];
	int err;

	err = i2c_burst_read_dt(&config->i2c_bus, RV8803C7_REGISTER_SECONDS, regs, sizeof(regs));
	if (err < 0) {
		return err;
	}

	// Check to confirm correct time
	if ((regs[0] & RV8803C7_SECONDS_BITS) == bin2bcd(59)) {
		err = i2c_burst_read_dt(&config->i2c_bus, RV8803C7_REGISTER_SECONDS, regs, sizeof(regs));
		if (err < 0) {
			return err;
		}
	}

	timeptr->tm_sec		= bcd2bin(regs[0] & RV8803C7_SECONDS_BITS);
	timeptr->tm_min		= bcd2bin(regs[1] & RV8803C7_MINUTES_BITS);
	timeptr->tm_hour	= bcd2bin(regs[2] & RV8803C7_HOURS_BITS);
	timeptr->tm_wday	= bcd2bin(regs[3] & RV8803C7_WEEKDAY_BITS);
	timeptr->tm_mday	= bcd2bin(regs[4] & RV8803C7_DATE_BITS);
	timeptr->tm_mon		= bcd2bin(regs[5] & RV8803C7_MONTH_BITS);
	timeptr->tm_year	= bcd2bin(regs[6] & RV8803C7_YEAR_BITS) + RV8803C7_CORRECT_YEAR_LEAP_MIN;

	LOG_DBG("Get time: year[%u] month[%u] mday[%u] wday[%u] hours[%u] minutes[%u] seconds[%u]",
			timeptr->tm_year,
			timeptr->tm_mon,
			timeptr->tm_mday,
			timeptr->tm_wday,
			timeptr->tm_hour,
			timeptr->tm_min,
			timeptr->tm_sec);
	return 0;
}

static int rv_8803_c7_init(const struct device *dev)
{
	const struct rv_8803_c7_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->i2c_bus)) {
		LOG_ERR("I2C bus not ready!!");
		return -ENODEV;
	}

	return 0;
}

static const struct rtc_driver_api rv_8803_c7_driver_api = {
	.set_time = rv_8803_c7_set_time,
	.get_time = rv_8803_c7_get_time,
};

#define RV_8803_C7_INIT(inst)																		\
	static struct rv_8803_c7_data rv_8803_c7_data_##inst;											\
	static const struct rv_8803_c7_config rv_8803_c7_config_##inst = {									\
		.i2c_bus = I2C_DT_SPEC_INST_GET(inst),														\
	};																								\
                                                                                                	\
	DEVICE_DT_INST_DEFINE(inst,																		\
			rv_8803_c7_init,																		\
			NULL,																					\
			&rv_8803_c7_data_##inst,																\
			&rv_8803_c7_config_##inst,																\
			POST_KERNEL,																			\
			CONFIG_RTC_INIT_PRIORITY,																\
			&rv_8803_c7_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RV_8803_C7_INIT)
