/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microcrystal_rv_8803_c7

#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <math.h>


LOG_MODULE_REGISTER(RV_8803_C7, CONFIG_RTC_LOG_LEVEL);

/* Registers */
#define RV8803C7_REGISTER_SECONDS   0x00
#define RV8803C7_REGISTER_MINUTES   0x01
#define RV8803C7_REGISTER_HOURS     0x02
#define RV8803C7_REGISTER_WEEKDAY   0x03
#define RV8803C7_REGISTER_DATE      0x04
#define RV8803C7_REGISTER_MONTH     0x05
#define RV8803C7_REGISTER_YEAR      0x06

/* Data masks */
#define RV8803C7_SECONDS_BITS   GENMASK(6, 0)
#define RV8803C7_MINUTES_BITS   GENMASK(6, 0)
#define RV8803C7_HOURS_BITS     GENMASK(5, 0)
#define RV8803C7_WEEKDAY_BITS   GENMASK(6, 0)
#define RV8803C7_DATE_BITS      GENMASK(5, 0)
#define RV8803C7_MONTH_BITS     GENMASK(4, 0)
#define RV8803C7_YEAR_BITS      GENMASK(7, 0)

/* TM OFFSET */
#define RV8803C7_TM_MONTH 1

/* Control MACRO */
#define RV8803C7_PARTIAL_SECONDS_INCR 59 // Check for partial incrementation when reads get 59 seconds
#define RV8803C7_CORRECT_YEAR_LEAP_MIN (2000 - 1900) // Diff between 2000 and tm base year 1900
#define RV8803C7_CORRECT_YEAR_LEAP_MAX (2099 - 1900) // Diff between 2099 and tm base year 1900

/* Structs */
struct rv8803c7_config {
	struct i2c_dt_spec i2c_bus;
};

struct rv8803c7_data {
};


/* API */
static int rv8803c7_set_time(const struct device *dev, const struct rtc_time *timeptr) {
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
	const struct rv8803c7_config *config = dev->config;
	uint8_t regs[8];

	regs[0] = RV8803C7_REGISTER_SECONDS;
	regs[1] = bin2bcd(timeptr->tm_sec) & RV8803C7_SECONDS_BITS;
	regs[2] = bin2bcd(timeptr->tm_min) & RV8803C7_MINUTES_BITS;
	regs[3] = bin2bcd(timeptr->tm_hour) & RV8803C7_HOURS_BITS;
	regs[4] = (1 << timeptr->tm_wday) & RV8803C7_WEEKDAY_BITS;
	regs[5] = bin2bcd(timeptr->tm_mday) & RV8803C7_DATE_BITS;
	regs[6] = bin2bcd(timeptr->tm_mon + RV8803C7_TM_MONTH) & RV8803C7_MONTH_BITS;
	regs[7] = bin2bcd(timeptr->tm_year - RV8803C7_CORRECT_YEAR_LEAP_MIN) & RV8803C7_YEAR_BITS;

	return i2c_write_dt(&config->i2c_bus, regs, 8);
}

static int rv8803c7_get_time(const struct device *dev, struct rtc_time *timeptr) {
	if (timeptr == NULL) {
		return -EINVAL;
	}

	const struct rv8803c7_config *config = dev->config;
	uint8_t regs1[7];
	uint8_t regs2[7];
	uint8_t *correct = regs1;
	int err;

	err = i2c_burst_read_dt(&config->i2c_bus, RV8803C7_REGISTER_SECONDS, regs1, sizeof(regs1));
	if (err < 0) {
		return err;
	}

	// Check to confirm correct time
	if ((regs1[0] & RV8803C7_SECONDS_BITS) == bin2bcd(59)) {
		err = i2c_burst_read_dt(&config->i2c_bus, RV8803C7_REGISTER_SECONDS, regs2, sizeof(regs2));
		if (err < 0) {
			return err;
		}
		if ((correct[0] & RV8803C7_SECONDS_BITS) != bin2bcd(59)) {
			correct = regs2;
		}
	}

	timeptr->tm_sec		= bcd2bin(correct[0] & RV8803C7_SECONDS_BITS);
	timeptr->tm_min		= bcd2bin(correct[1] & RV8803C7_MINUTES_BITS);
	timeptr->tm_hour	= bcd2bin(correct[2] & RV8803C7_HOURS_BITS);
	timeptr->tm_wday	= log2(correct[3] & RV8803C7_WEEKDAY_BITS);
	timeptr->tm_mday	= bcd2bin(correct[4] & RV8803C7_DATE_BITS);
	timeptr->tm_mon		= bcd2bin(correct[5] & RV8803C7_MONTH_BITS) - RV8803C7_TM_MONTH;
	timeptr->tm_year	= bcd2bin(correct[6] & RV8803C7_YEAR_BITS) + RV8803C7_CORRECT_YEAR_LEAP_MIN;

	// Unused
	timeptr->tm_nsec	= 0;
	timeptr->tm_isdst	= -1;
	timeptr->tm_yday	= -1;

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

static int rv8803c7_init(const struct device *dev)
{
	const struct rv8803c7_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->i2c_bus)) {
		LOG_ERR("I2C bus not ready!!");
		return -ENODEV;
	}

	return 0;
}

static const struct rtc_driver_api rv8803c7_driver_api = {
	.set_time = rv8803c7_set_time,
	.get_time = rv8803c7_get_time,
};

/*#define RV8803C7_INIT(inst)																		\
	static struct rv8803c7_data rv8803c7_data_##inst;											\
	static const struct rv8803c7_config rv8803c7_config_##inst = {									\
		.i2c_bus = I2C_DT_SPEC_INST_GET(inst),														\
	};																								\
                                                                                                	\
	DEVICE_DT_INST_DEFINE(inst,																		\
			&rv8803c7_init,																		\
			NULL,																					\
			&rv8803c7_data_##inst,																\
			&rv8803c7_config_##inst,																\
			POST_KERNEL,																			\
			CONFIG_RTC_INIT_PRIORITY,																\
			&rv8803c7_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RV8803C7_INIT)*/

#define RV8803C7_INIT(n)                                                                             \
	static const struct rv8803c7_config rv8803c7_config_##n = {                                          \
		.i2c_bus = I2C_DT_SPEC_INST_GET(n),                                                    \
	};                                                                                         \
	static struct rv8803c7_data rv8803c7_data_##n;                                                 \
	DEVICE_DT_INST_DEFINE(n, rv8803c7_init, NULL, &rv8803c7_data_##n, &rv8803c7_config_##n,          \
			      POST_KERNEL, CONFIG_RTC_INIT_PRIORITY, &rv8803c7_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RV8803C7_INIT)