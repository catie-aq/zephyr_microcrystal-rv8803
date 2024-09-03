/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microcrystal_rv8803

#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <math.h>


LOG_MODULE_REGISTER(RV8803, CONFIG_RTC_LOG_LEVEL);

/* Registers */
#define RV8803_REGISTER_SECONDS	0x00
#define RV8803_REGISTER_MINUTES	0x01
#define RV8803_REGISTER_HOURS	0x02
#define RV8803_REGISTER_WEEKDAY	0x03
#define RV8803_REGISTER_DATE	0x04
#define RV8803_REGISTER_MONTH	0x05
#define RV8803_REGISTER_YEAR	0x06
#define RV8803_REGISTER_CONTROL	0x0F

/* Data masks */
#define RV8803_SECONDS_BITS	GENMASK(6, 0)
#define RV8803_MINUTES_BITS	GENMASK(6, 0)
#define RV8803_HOURS_BITS	GENMASK(5, 0)
#define RV8803_WEEKDAY_BITS	GENMASK(6, 0)
#define RV8803_DATE_BITS	GENMASK(5, 0)
#define RV8803_MONTH_BITS	GENMASK(4, 0)
#define RV8803_YEAR_BITS	GENMASK(7, 0)

/* TM OFFSET */
#define RV8803_TM_MONTH	1

/* Control MACRO */
#define RV8803_PARTIAL_SECONDS_INCR		59 // Check for partial incrementation when reads get 59 seconds
#define RV8803_CORRECT_YEAR_LEAP_MIN	(2000 - 1900) // Diff between 2000 and tm base year 1900
#define RV8803_CORRECT_YEAR_LEAP_MAX	(2099 - 1900) // Diff between 2099 and tm base year 1900
#define RV8803_RESET_BIT				0x01

/* Structs */
struct rv8803_config {
	struct i2c_dt_spec i2c_bus;
};

struct rv8803_data {
};


/* API */
static int rv8803_set_time(const struct device *dev, const struct rtc_time *timeptr) {
	// Valid date are between 2000 and 2099
	if ((timeptr == NULL) || (timeptr->tm_year < RV8803_CORRECT_YEAR_LEAP_MIN) || (timeptr->tm_year > RV8803_CORRECT_YEAR_LEAP_MAX)) {
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
	const struct rv8803_config *config = dev->config;
	uint8_t regs[7];
	uint8_t control_reg[1];
	int err;

	regs[0] = bin2bcd(timeptr->tm_sec) & RV8803_SECONDS_BITS;
	regs[1] = bin2bcd(timeptr->tm_min) & RV8803_MINUTES_BITS;
	regs[2] = bin2bcd(timeptr->tm_hour) & RV8803_HOURS_BITS;
	regs[3] = (1 << timeptr->tm_wday) & RV8803_WEEKDAY_BITS;
	regs[4] = bin2bcd(timeptr->tm_mday) & RV8803_DATE_BITS;
	regs[5] = bin2bcd(timeptr->tm_mon + RV8803_TM_MONTH) & RV8803_MONTH_BITS;
	regs[6] = bin2bcd(timeptr->tm_year - RV8803_CORRECT_YEAR_LEAP_MIN) & RV8803_YEAR_BITS;

	// Stopping time update clock
    err = i2c_burst_read_dt(&config->i2c_bus, RV8803_REGISTER_CONTROL, control_reg, sizeof(control_reg));
	if (err < 0) {
		return err;
	}
	control_reg[0] |= RV8803_RESET_BIT;
	err = i2c_burst_write_dt(&config->i2c_bus, RV8803_REGISTER_CONTROL, control_reg, sizeof(control_reg));
	if (err < 0) {
		return err;
	}

	// Write new time to RTC register
	i2c_burst_write_dt(&config->i2c_bus, RV8803_REGISTER_SECONDS, regs, sizeof(regs));

	// Restart time update clock
	control_reg[0] &= ~RV8803_RESET_BIT;
    return i2c_burst_write_dt(&config->i2c_bus, RV8803_REGISTER_CONTROL, control_reg, sizeof(control_reg));
}

static int rv8803_get_time(const struct device *dev, struct rtc_time *timeptr) {
	if (timeptr == NULL) {
		return -EINVAL;
	}

	// Init variables for i2c communication
	const struct rv8803_config *config = dev->config;
	uint8_t regs1[7];
	uint8_t regs2[7];
	uint8_t *correct = regs1;
	int err;

	err = i2c_burst_read_dt(&config->i2c_bus, RV8803_REGISTER_SECONDS, regs1, sizeof(regs1));
	if (err < 0) {
		return err;
	}

	// Check to confirm correct time
	if ((regs1[0] & RV8803_SECONDS_BITS) == bin2bcd(59)) {
		err = i2c_burst_read_dt(&config->i2c_bus, RV8803_REGISTER_SECONDS, regs2, sizeof(regs2));
		if (err < 0) {
			return err;
		}
		if ((regs2[0] & RV8803_SECONDS_BITS) != bin2bcd(59)) {
			correct = regs2;
		}
	}

	timeptr->tm_sec		= bcd2bin(correct[0] & RV8803_SECONDS_BITS);
	timeptr->tm_min		= bcd2bin(correct[1] & RV8803_MINUTES_BITS);
	timeptr->tm_hour	= bcd2bin(correct[2] & RV8803_HOURS_BITS);
	timeptr->tm_wday	= log2(correct[3] & RV8803_WEEKDAY_BITS);
	timeptr->tm_mday	= bcd2bin(correct[4] & RV8803_DATE_BITS);
	timeptr->tm_mon		= bcd2bin(correct[5] & RV8803_MONTH_BITS) - RV8803_TM_MONTH;
	timeptr->tm_year	= bcd2bin(correct[6] & RV8803_YEAR_BITS) + RV8803_CORRECT_YEAR_LEAP_MIN;

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

static int rv8803_init(const struct device *dev)
{
	const struct rv8803_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->i2c_bus)) {
		LOG_ERR("I2C bus not ready!!");
		return -ENODEV;
	}

	return 0;
}

static const struct rtc_driver_api rv8803_driver_api = {
	.set_time = rv8803_set_time,
	.get_time = rv8803_get_time,
};

#define RV8803_INIT(n)                                                                             \
	static const struct rv8803_config rv8803_config_##n = {                                          \
		.i2c_bus = I2C_DT_SPEC_INST_GET(n),                                                    \
	};                                                                                         \
	static struct rv8803_data rv8803_data_##n;                                                 \
	DEVICE_DT_INST_DEFINE(n, rv8803_init, NULL, &rv8803_data_##n, &rv8803_config_##n,          \
			      POST_KERNEL, CONFIG_RTC_INIT_PRIORITY, &rv8803_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RV8803_INIT)