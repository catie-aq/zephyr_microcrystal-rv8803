/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microcrystal_rv8803

#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <math.h>


LOG_MODULE_REGISTER(RV8803, CONFIG_RTC_LOG_LEVEL);

/* Calendar Registers */
#define RV8803_REGISTER_SECONDS	0x00
#define RV8803_REGISTER_MINUTES	0x01
#define RV8803_REGISTER_HOURS	0x02
#define RV8803_REGISTER_WEEKDAY	0x03
#define RV8803_REGISTER_DATE	0x04
#define RV8803_REGISTER_MONTH	0x05
#define RV8803_REGISTER_YEAR	0x06

/* Alarm Registers */
#define RV8803_REGISTER_ALARM_MINUTES	0x08
#define RV8803_REGISTER_ALARM_HOURS		0x09
#define RV8803_REGISTER_ALARM_WADA		0x0A

/* Control Registers */
#define RV8803_REGISTER_EXTENSION	0x0D
#define RV8803_REGISTER_FLAG		0x0E
#define RV8803_REGISTER_CONTROL		0x0F

/* Data masks */
#define RV8803_SECONDS_BITS	GENMASK(6, 0)
#define RV8803_MINUTES_BITS	GENMASK(6, 0)
#define RV8803_HOURS_BITS	GENMASK(5, 0)
#define RV8803_WEEKDAY_BITS	GENMASK(6, 0)
#define RV8803_DATE_BITS	GENMASK(5, 0)
#define RV8803_MONTH_BITS	GENMASK(4, 0)
#define RV8803_YEAR_BITS	GENMASK(7, 0)

/* Registers Mask */
#define RV8803_EXTENSION_MASK_WADA		(0x01 << 6)
#define RV8803_FLAG_MASK_ALARM			(0x01 << 3)
#define RV8803_CONTROL_MASK_ALARM		(0x01 << 3)
#define RV8803_ALARM_ENABLE_MINUTES		(0x00 << 7)
#define RV8803_ALARM_ENABLE_HOURS		(0x00 << 7)
#define RV8803_ALARM_ENABLE_WADA		(0x00 << 7)
#define RV8803_ALARM_DISABLE_MINUTES	(0x01 << 7)
#define RV8803_ALARM_DISABLE_HOURS		(0x01 << 7)
#define RV8803_ALARM_DISABLE_WADA		(0x01 << 7)
#define RV8803_ALARM_MASK_MINUTES		RV8803_ALARM_DISABLE_MINUTES
#define RV8803_ALARM_MASK_HOURS			RV8803_ALARM_DISABLE_HOURS
#define RV8803_ALARM_MASK_WADA			RV8803_ALARM_DISABLE_WADA

/* TM OFFSET */
#define RV8803_TM_MONTH	1

/* Control MACRO */
#define RV8803_PARTIAL_SECONDS_INCR		59 // Check for partial incrementation when reads get 59 seconds
#define RV8803_CORRECT_YEAR_LEAP_MIN	(2000 - 1900) // Diff between 2000 and tm base year 1900
#define RV8803_CORRECT_YEAR_LEAP_MAX	(2099 - 1900) // Diff between 2099 and tm base year 1900
#define RV8803_RESET_BIT				(0x01 << 0)
#define RV8803_ENABLE_ALARM				(0x01 << 3)
#define RV8803_DISABLE_ALARM			(0x00 << 3)
#define RV8803_WEEKDAY_ALARM			(0x00 << 6)
#define RV8803_MONTHDAY_ALARM			(0x01 << 6)

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(irq_gpios)
#if defined(CONFIG_RTC_ALARM)
#define RV8803_IRQ_GPIO_USE_ALARM 1
#endif
#if defined(CONFIG_RTC_UPDATE)
#define RV8803_IRQ_GPIO_USE_UPDATE 1
#endif
#endif

#if defined(RV8803_IRQ_GPIO_USE_ALARM) || defined(RV8803_IRQ_GPIO_USE_UPDATE)
#define RV8803_IRQ_GPIO_IN_USE 1
#endif

/* Structs */
struct rv8803_config {
	struct i2c_dt_spec i2c_bus;
#if RV8803_IRQ_GPIO_IN_USE
	const struct gpio_dt_spec irq_gpio;
#endif
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

#if RV8803_IRQ_GPIO_USE_ALARM
static bool rv8803_alarm_time_valid(const struct rtc_time *timeptr, uint16_t mask)
{
	if (timeptr->tm_sec != 0) {
		return false;
	}

	if ((mask & RTC_ALARM_TIME_MASK_MINUTE) && (timeptr->tm_min < 0 || timeptr->tm_min > 59)) {
		return false;
	}

	if ((mask & RTC_ALARM_TIME_MASK_HOUR) && (timeptr->tm_hour < 0 || timeptr->tm_hour > 23)) {
		return false;
	}

	if (timeptr->tm_mon != 0) {
		return false;
	}

	if ((mask & RTC_ALARM_TIME_MASK_MONTHDAY) && (mask & RTC_ALARM_TIME_MASK_WEEKDAY)) {
		return false;
	}

	if ((mask & RTC_ALARM_TIME_MASK_MONTHDAY) &&
	    (timeptr->tm_mday < 1 || timeptr->tm_mday > 31)) {
		return false;
	}

	if ((mask & RTC_ALARM_TIME_MASK_WEEKDAY) &&
	    (timeptr->tm_wday < 0 || timeptr->tm_wday > 6)) {
		return false;
	}

	return true;
}

static int rv8803_alarm_get_supported_fields(const struct device *dev, uint16_t id, uint16_t *mask)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(id);

	(*mask) = 	(RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |
				RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_WEEKDAY);

	return 0;
}
static int rv8803_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask, const struct rtc_time *timeptr)
{
	ARG_UNUSED(id);
	const struct rv8803_config *config = dev->config;
	int err;

	if ((timeptr == NULL) && (mask > 0)) {
		LOG_ERR("Invalid time pointer!!");
		return -EINVAL;
	}

	if (!rv8803_alarm_time_valid(timeptr, mask)) {
		LOG_ERR("Invalid Time / Mask!!");
		return -EINVAL;
	}

	// Mask = 0 : Remove alarm interrupt
	if (mask == 0) {
		err = i2c_reg_update_byte_dt(&config->i2c_bus,
									RV8803_REGISTER_CONTROL,
					    			RV8803_CONTROL_MASK_ALARM,
					    			RV8803_DISABLE_ALARM);
		if (err < 0) return err;
		err = i2c_reg_update_byte_dt(&config->i2c_bus,
									RV8803_REGISTER_FLAG,
					    			RV8803_FLAG_MASK_ALARM,
					    			RV8803_DISABLE_ALARM);
		if (err < 0) return err;

		return 0;
	}

	// AIE and AF to 0 -> stop interrupt and clear interrupt flags
	err = i2c_reg_update_byte_dt(&config->i2c_bus,
								RV8803_REGISTER_CONTROL,
								RV8803_CONTROL_MASK_ALARM,
								RV8803_DISABLE_ALARM);
	if (err < 0) return err;
	err = i2c_reg_update_byte_dt(&config->i2c_bus,
								RV8803_REGISTER_FLAG,
								RV8803_FLAG_MASK_ALARM,
								RV8803_DISABLE_ALARM);
	if (err < 0) return err;

	// Set WADA to 0 or 1
	uint8_t wada = RV8803_WEEKDAY_ALARM;
	if (mask && RTC_ALARM_TIME_MASK_MONTHDAY) {
		wada = RV8803_MONTHDAY_ALARM;
	}
	err = i2c_reg_update_byte_dt(&config->i2c_bus,
								RV8803_REGISTER_EXTENSION,
								RV8803_EXTENSION_MASK_WADA,
								wada);
	if (err < 0) return err;

	// Set desired time and alarm
	uint8_t regs[3];
	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		regs[0] = RV8803_ALARM_ENABLE_MINUTES;
		regs[0] |= bin2bcd(timeptr->tm_min) & RV8803_MINUTES_BITS;
	} else {
		regs[0] = RV8803_ALARM_DISABLE_MINUTES;
	}

	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		regs[1] = RV8803_ALARM_ENABLE_HOURS;
		regs[1] |= bin2bcd(timeptr->tm_hour) & RV8803_HOURS_BITS;
	} else {
		regs[1] = RV8803_ALARM_DISABLE_HOURS;
	}

	if (mask & RTC_ALARM_TIME_MASK_WEEKDAY) {
		regs[2] = RV8803_ALARM_ENABLE_WADA;
		regs[2] |= (1 << timeptr->tm_wday) & RV8803_WEEKDAY_BITS;
	} else if (mask && RTC_ALARM_TIME_MASK_MONTHDAY) {
		regs[2] = RV8803_ALARM_ENABLE_WADA;
		regs[2] |= bin2bcd(timeptr->tm_mday) & RV8803_DATE_BITS;
	} else {
		regs[2] = RV8803_ALARM_DISABLE_WADA;
	}

	err = i2c_burst_write_dt(&config->i2c_bus, RV8803_REGISTER_ALARM_MINUTES, regs, sizeof(regs));
	if (err < 0) return err;

	// AIE 1 -> activate interrupt
	err = i2c_reg_update_byte_dt(&config->i2c_bus,
								RV8803_REGISTER_CONTROL,
								RV8803_CONTROL_MASK_ALARM,
								RV8803_ENABLE_ALARM);
	if (err < 0) return err;

	return 0;
}

static int rv8803_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask, struct rtc_time *timeptr)
{
	ARG_UNUSED(id);
	const struct rv8803_config *config = dev->config;
	int err;

	if (timeptr == NULL) {
		return -EINVAL;
	}
	(*mask) = 0;

	uint8_t regs[3];
	err = i2c_burst_read_dt(&config->i2c_bus, RV8803_REGISTER_ALARM_MINUTES, regs, sizeof(regs));
	if (err < 0) return err;

	if ((regs[0] & RV8803_ALARM_MASK_MINUTES) == RV8803_ALARM_ENABLE_MINUTES) {
		(*mask) |= RTC_ALARM_TIME_MASK_MINUTE;
		timeptr->tm_min = bcd2bin(regs[0] & RV8803_MINUTES_BITS);
	}

	if ((regs[1] & RV8803_ALARM_MASK_HOURS) == RV8803_ALARM_ENABLE_HOURS) {
		(*mask) |= RTC_ALARM_TIME_MASK_HOUR;
		timeptr->tm_hour = bcd2bin(regs[1] & RV8803_HOURS_BITS);
	}

	if ((regs[2] & RV8803_ALARM_MASK_WADA) == RV8803_ALARM_ENABLE_WADA) {
		uint8_t wada;
		err = i2c_reg_read_byte_dt(&config->i2c_bus, RV8803_REGISTER_EXTENSION, &wada);
		if (err < 0) return err;

		if ((wada & RV8803_EXTENSION_MASK_WADA) == RV8803_WEEKDAY_ALARM) {
			(*mask) |= RTC_ALARM_TIME_MASK_WEEKDAY;
			timeptr->tm_wday = log2(regs[2] & RV8803_WEEKDAY_BITS);
		} else {
			(*mask) |= RTC_ALARM_TIME_MASK_MONTHDAY;
			timeptr->tm_mday = bcd2bin(regs[2] & RV8803_DATE_BITS);
		}
	}

	return 0;
}

static int rv8803_alarm_is_pending(const struct device *dev, uint16_t id)
{
	ARG_UNUSED(id);
	const struct rv8803_config *config = dev->config;
	uint8_t reg;
	int err;

	err = i2c_reg_read_byte_dt(&config->i2c_bus, RV8803_REGISTER_FLAG, &reg);
	if (err < 0) return err;

	if (reg & RV8803_FLAG_MASK_ALARM) {
		err = i2c_reg_update_byte_dt(&config->i2c_bus,
								RV8803_REGISTER_FLAG,
								RV8803_FLAG_MASK_ALARM,
								RV8803_FLAG_MASK_ALARM);
		if (err < 0) return err;

		return 1;
	}

	return 0;
}

static int rv8803_alarm_set_callback(const struct device *dev, uint16_t id, rtc_alarm_callback callback, void *user_data)
{
	return 0;
}

#endif // CONFIG_RTC_ALARM

static int rv8803_init(const struct device *dev)
{
	const struct rv8803_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->i2c_bus)) {
		LOG_ERR("I2C bus not ready!!");
		return -ENODEV;
	}
#if RV8803_IRQ_GPIO_IN_USE
	LOG_INF("IRQ handle: [%s][%d][%d]!", config->irq_gpio.port->name, config->irq_gpio.pin, config->irq_gpio.dt_flags);
#endif

	return 0;
}

static const struct rtc_driver_api rv8803_driver_api = {
	.set_time = rv8803_set_time,
	.get_time = rv8803_get_time,
#if CONFIG_RTC_ALARM
	.alarm_get_supported_fields = rv8803_alarm_get_supported_fields,
	.alarm_set_time				= rv8803_alarm_set_time,
	.alarm_get_time				= rv8803_alarm_get_time,
	.alarm_is_pending			= rv8803_alarm_is_pending,
	.alarm_set_callback			= rv8803_alarm_set_callback,
#endif
};

#define RV8803_INIT(n)                                                                         \
	static const struct rv8803_config rv8803_config_##n = {                                    \
		.i2c_bus = I2C_DT_SPEC_INST_GET(n),                                                    \
		IF_ENABLED(RV8803_IRQ_GPIO_IN_USE,                                                     \
		(.irq_gpio = GPIO_DT_SPEC_INST_GET_OR(n, irq_gpios, {0}))),                            \
	};                                                                                         \
	static struct rv8803_data rv8803_data_##n;                                                 \
	DEVICE_DT_INST_DEFINE(n, rv8803_init, NULL, &rv8803_data_##n, &rv8803_config_##n,          \
			      POST_KERNEL, CONFIG_RTC_INIT_PRIORITY, &rv8803_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RV8803_INIT)