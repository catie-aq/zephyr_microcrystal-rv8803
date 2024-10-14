/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microcrystal_rv8803_rtc

#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include "rv8803.h"
#include "rv8803_rtc.h"
#include <math.h>

LOG_MODULE_REGISTER(RV8803_RTC, CONFIG_RTC_LOG_LEVEL);

#if CONFIG_RTC && CONFIG_RV8803_RTC_ENABLE
/* API */
static int rv8803_rtc_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	/* Valid date are between 2000 and 2099 */
	if ((timeptr == NULL) || (timeptr->tm_year < RV8803_CORRECT_YEAR_LEAP_MIN) ||
	    (timeptr->tm_year > RV8803_CORRECT_YEAR_LEAP_MAX)) {
		LOG_ERR("invalid time");
		return -EINVAL;
	}

	LOG_DBG("Set time: year[%u] month[%u] mday[%u] wday[%u] hours[%u] minutes[%u] seconds[%u]",
		timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday,
		timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);

	/* Init variables for i2c communication */
	const struct rv8803_rtc_config *rtc_config = dev->config;
	const struct rv8803_config *config = rtc_config->base_dev->config;
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

	/* Stopping time update clock */
	err = i2c_burst_read_dt(&config->i2c_bus, RV8803_REGISTER_CONTROL, control_reg,
				sizeof(control_reg));
	if (err < 0) {
		return err;
	}
	control_reg[0] |= RV8803_RESET_BIT;
	err = i2c_burst_write_dt(&config->i2c_bus, RV8803_REGISTER_CONTROL, control_reg,
				 sizeof(control_reg));
	if (err < 0) {
		return err;
	}

	/* Write new time to RTC register */
	i2c_burst_write_dt(&config->i2c_bus, RV8803_REGISTER_SECONDS, regs, sizeof(regs));

	/* Restart time update clock */
	control_reg[0] &= ~RV8803_RESET_BIT;
	return i2c_burst_write_dt(&config->i2c_bus, RV8803_REGISTER_CONTROL, control_reg,
				  sizeof(control_reg));
}

static int rv8803_rtc_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	if (timeptr == NULL) {
		return -EINVAL;
	}

	/* Init variables for i2c communication */
	const struct rv8803_rtc_config *rtc_config = dev->config;
	const struct rv8803_config *config = rtc_config->base_dev->config;
	uint8_t regs1[7];
	uint8_t regs2[7];
	uint8_t *correct = regs1;
	int err;

	err = i2c_burst_read_dt(&config->i2c_bus, RV8803_REGISTER_SECONDS, regs1, sizeof(regs1));
	if (err < 0) {
		return err;
	}

	/* Check to confirm correct time */
	if ((regs1[0] & RV8803_SECONDS_BITS) == bin2bcd(59)) {
		err = i2c_burst_read_dt(&config->i2c_bus, RV8803_REGISTER_SECONDS, regs2,
					sizeof(regs2));
		if (err < 0) {
			return err;
		}
		if ((regs2[0] & RV8803_SECONDS_BITS) != bin2bcd(59)) {
			correct = regs2;
		}
	}

	timeptr->tm_sec = bcd2bin(correct[0] & RV8803_SECONDS_BITS);
	timeptr->tm_min = bcd2bin(correct[1] & RV8803_MINUTES_BITS);
	timeptr->tm_hour = bcd2bin(correct[2] & RV8803_HOURS_BITS);
	timeptr->tm_wday = log2(correct[3] & RV8803_WEEKDAY_BITS);
	timeptr->tm_mday = bcd2bin(correct[4] & RV8803_DATE_BITS);
	timeptr->tm_mon = bcd2bin(correct[5] & RV8803_MONTH_BITS) - RV8803_TM_MONTH;
	timeptr->tm_year = bcd2bin(correct[6] & RV8803_YEAR_BITS) + RV8803_CORRECT_YEAR_LEAP_MIN;

	/* Unused */
	timeptr->tm_nsec = 0;
	timeptr->tm_isdst = -1;
	timeptr->tm_yday = -1;

	LOG_DBG("Get time: year[%u] month[%u] mday[%u] wday[%u] hours[%u] minutes[%u] seconds[%u]",
		timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday,
		timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);
	return 0;
}

#if RV8803_IRQ_GPIO_IN_USE
static void rv8803_rtc_worker(struct k_work *p_work)
{
	struct rv8803_data *data = CONTAINER_OF(p_work, struct rv8803_data, rtc_work);
	const struct rv8803_rtc_config *rtc_config = data->rtc_dev->config;
	const struct rv8803_rtc_data *rtc_data = data->rtc_dev->data;
	const struct rv8803_config *config = rtc_config->base_dev->config;
	uint8_t reg;
	int err;

	LOG_DBG("Process Alarm worker from interrupt");

	err = i2c_reg_read_byte_dt(&config->i2c_bus, RV8803_REGISTER_FLAG, &reg);
	if (err < 0) {
		LOG_ERR("Alarm worker I2C read FLAGS error");
	}

#if RV8803_IRQ_GPIO_USE_ALARM
	if (reg & RV8803_FLAG_MASK_ALARM) {
		if (rtc_data->alarm_cb != NULL) {
			LOG_DBG("Calling Alarm callback");
			rtc_data->alarm_cb(data->rtc_dev, 0, rtc_data->alarm_cb_data);

			err = i2c_reg_update_byte_dt(&config->i2c_bus, RV8803_REGISTER_FLAG,
						     RV8803_FLAG_MASK_ALARM, RV8803_DISABLE_ALARM);
			if (err < 0) {
				LOG_ERR("GPIO worker I2C update ALARM FLAG error");
			}
		}
	}
#endif

#if RV8803_IRQ_GPIO_USE_UPDATE
	if (reg & RV8803_FLAG_MASK_UPDATE) {
		if (rtc_data->update_cb != NULL) {
			LOG_DBG("Calling Update callback");
			rtc_data->update_cb(data->rtc_dev, rtc_data->update_cb_data);

			err = i2c_reg_update_byte_dt(&config->i2c_bus, RV8803_REGISTER_FLAG,
						     RV8803_FLAG_MASK_UPDATE,
						     RV8803_DISABLE_UPDATE);
			if (err < 0) {
				LOG_ERR("GPIO worker I2C update UPDATE FLAG error");
			}
		}
	}
#endif
}
#endif

#if RV8803_IRQ_GPIO_USE_ALARM
static bool rv8803_rtc_alarm_time_valid(const struct rtc_time *timeptr, uint16_t mask)
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

static int rv8803_rtc_alarm_get_supported_fields(const struct device *dev, uint16_t id,
						 uint16_t *mask)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(id);

	(*mask) = (RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |
		   RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_WEEKDAY);

	return 0;
}
static int rv8803_rtc_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				     const struct rtc_time *timeptr)
{
	ARG_UNUSED(id);
	const struct rv8803_rtc_config *rtc_config = dev->config;
	const struct rv8803_config *config = rtc_config->base_dev->config;
	int err;

	if ((timeptr == NULL) && (mask > 0)) {
		LOG_ERR("Invalid time pointer!!");
		return -EINVAL;
	}

	if (!rv8803_rtc_alarm_time_valid(timeptr, mask)) {
		LOG_ERR("Invalid Time / Mask!!");
		return -EINVAL;
	}

	/* Mask = 0 : Remove alarm interrupt */
	if (mask == 0) {
		err = i2c_reg_update_byte_dt(&config->i2c_bus, RV8803_REGISTER_CONTROL,
					     RV8803_CONTROL_MASK_ALARM, RV8803_DISABLE_ALARM);
		if (err < 0) {
			LOG_ERR("Update CONTROL: [%d]", err);
			return err;
		}
		err = i2c_reg_update_byte_dt(&config->i2c_bus, RV8803_REGISTER_FLAG,
					     RV8803_FLAG_MASK_ALARM, RV8803_DISABLE_ALARM);
		if (err < 0) {
			LOG_ERR("Update FLAG: [%d]", err);
			return err;
		}

		return 0;
	}

	/* AIE and AF to 0 -> stop interrupt and clear interrupt flags */
	err = i2c_reg_update_byte_dt(&config->i2c_bus, RV8803_REGISTER_CONTROL,
				     RV8803_CONTROL_MASK_ALARM, RV8803_DISABLE_ALARM);
	if (err < 0) {
		LOG_ERR("Update CONTROL: [%d]", err);
		return err;
	}
	err = i2c_reg_update_byte_dt(&config->i2c_bus, RV8803_REGISTER_FLAG, RV8803_FLAG_MASK_ALARM,
				     RV8803_DISABLE_ALARM);
	if (err < 0) {
		LOG_ERR("Update FLAG: [%d]", err);
		return err;
	}

	/* Set WADA to 0 or 1 */
	uint8_t wada = RV8803_WEEKDAY_ALARM;
	if (mask && RTC_ALARM_TIME_MASK_MONTHDAY) {
		wada = RV8803_MONTHDAY_ALARM;
	}
	err = i2c_reg_update_byte_dt(&config->i2c_bus, RV8803_REGISTER_EXTENSION,
				     RV8803_EXTENSION_MASK_WADA, wada);
	if (err < 0) {
		LOG_ERR("Update EXTENSION: [%d]", err);
		return err;
	}

	/* Set desired time and alarm */
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
	} else if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		regs[2] = RV8803_ALARM_ENABLE_WADA;
		regs[2] |= bin2bcd(timeptr->tm_mday) & RV8803_DATE_BITS;
	} else {
		regs[2] = RV8803_ALARM_DISABLE_WADA;
	}

	err = i2c_burst_write_dt(&config->i2c_bus, RV8803_REGISTER_ALARM_MINUTES, regs,
				 sizeof(regs));
	if (err < 0) {
		LOG_ERR("Write ALARM: [%d]", err);
		return err;
	}

	/* AIE 1 -> activate interrupt */
	err = i2c_reg_update_byte_dt(&config->i2c_bus, RV8803_REGISTER_CONTROL,
				     RV8803_CONTROL_MASK_ALARM, RV8803_ENABLE_ALARM);
	if (err < 0) {
		LOG_ERR("Update CONTROL: [%d]", err);
		return err;
	}

	return 0;
}

static int rv8803_rtc_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				     struct rtc_time *timeptr)
{
	ARG_UNUSED(id);
	const struct rv8803_rtc_config *rtc_config = dev->config;
	const struct rv8803_config *config = rtc_config->base_dev->config;
	int err;

	if (timeptr == NULL) {
		return -EINVAL;
	}
	(*mask) = 0;

	uint8_t regs[3];
	err = i2c_burst_read_dt(&config->i2c_bus, RV8803_REGISTER_ALARM_MINUTES, regs,
				sizeof(regs));
	if (err < 0) {
		return err;
	}

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
		if (err < 0) {
			return err;
		}

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

static int rv8803_rtc_alarm_is_pending(const struct device *dev, uint16_t id)
{
	ARG_UNUSED(id);
	const struct rv8803_rtc_config *rtc_config = dev->config;
	const struct rv8803_config *config = rtc_config->base_dev->config;
	uint8_t reg;
	int err;

	err = i2c_reg_read_byte_dt(&config->i2c_bus, RV8803_REGISTER_FLAG, &reg);
	if (err < 0) {
		return err;
	}

	if (reg & RV8803_FLAG_MASK_ALARM) {
		err = i2c_reg_update_byte_dt(&config->i2c_bus, RV8803_REGISTER_FLAG,
					     RV8803_FLAG_MASK_ALARM, RV8803_DISABLE_ALARM);
		if (err < 0) {
			return err;
		}

		return 1;
	}

	return 0;
}

static int rv8803_rtc_alarm_set_callback(const struct device *dev, uint16_t id,
					 rtc_alarm_callback callback, void *user_data)
{
	ARG_UNUSED(id);
	const struct rv8803_rtc_config *rtc_config = dev->config;
	const struct rv8803_config *config = rtc_config->base_dev->config;
	if (config->irq_gpio.port == NULL) {
		return -ENOTSUP;
	}

	struct rv8803_rtc_data *data = dev->data;
	data->alarm_cb = callback;
	data->alarm_cb_data = user_data;

	return 0;
}
#endif /* CONFIG_RTC_ALARM */
#endif

#if RV8803_IRQ_GPIO_USE_UPDATE
static int rv8803_setup_update_interrupt(const struct device *dev, bool disable)
{
	const struct rv8803_rtc_config *rtc_config = dev->config;
	const struct rv8803_config *config = rtc_config->base_dev->config;
	int err;

	/* UIE and UF to 0 : stop interrupt */
	err = i2c_reg_update_byte_dt(&config->i2c_bus, RV8803_REGISTER_CONTROL,
				     RV8803_CONTROL_MASK_UPDATE, RV8803_DISABLE_UPDATE);
	if (err < 0) {
		return err;
	}
	err = i2c_reg_update_byte_dt(&config->i2c_bus, RV8803_REGISTER_FLAG,
				     RV8803_FLAG_MASK_UPDATE, RV8803_DISABLE_UPDATE);
	if (err < 0) {
		return err;
	}

	if (disable) {
		return 0;
	}

	/* Choose USEL value */
	err = i2c_reg_update_byte_dt(&config->i2c_bus, RV8803_REGISTER_EXTENSION,
				     RV8803_EXTENSION_MASK_UPDATE, RV8803_DISABLE_UPDATE);
	if (err < 0) {
		return err;
	}

	/* UIE to 1 : start interrupt */
	err = i2c_reg_update_byte_dt(&config->i2c_bus, RV8803_REGISTER_CONTROL,
				     RV8803_CONTROL_MASK_UPDATE, RV8803_ENABLE_UPDATE);
	if (err < 0) {
		return err;
	}

	return 0;
}

static int rv8803_update_set_callback(const struct device *dev, rtc_update_callback callback,
				      void *user_data)
{
	const struct rv8803_rtc_config *rtc_config = dev->config;
	const struct rv8803_config *config = rtc_config->base_dev->config;
	if (config->irq_gpio.port == NULL) {
		return -ENOTSUP;
	}

	struct rv8803_rtc_data *data = dev->data;
	data->update_cb = callback;
	data->update_cb_data = user_data;

	if ((callback == NULL) && (user_data != NULL)) {
		return -EINVAL;
	}
	int err = rv8803_setup_update_interrupt(dev, (callback == NULL) && (user_data == NULL));
	if (err < 0) {
		return err;
	}

	return 0;
}
#endif /* CONFIG_RTC */

#if CONFIG_RTC && CONFIG_RV8803_RTC_ENABLE
/* RV8803 RTC init */
static int rv8803_rtc_init(const struct device *dev)
{
	const struct rv8803_rtc_config *rtc_config = dev->config;

	if (!device_is_ready(rtc_config->base_dev)) {
		return -ENODEV;
	}
	LOG_INF("RV8803 RTC INIT");

#if RV8803_IRQ_GPIO_IN_USE
	struct rv8803_data *data = rtc_config->base_dev->data;
	struct rv8803_rtc_data *rtc_data = dev->data;

	rtc_data->dev = dev;
	data->rtc_dev = dev;
	data->rtc_work.handler = rv8803_rtc_worker;

#if RV8803_IRQ_GPIO_USE_ALARM
	LOG_INF("RTC ALARM INIT");
	rtc_data->alarm_cb = NULL;
	rtc_data->alarm_cb_data = NULL;
#endif /* RV8803_IRQ_GPIO_USE_ALARM */
#if RV8803_IRQ_GPIO_USE_UPDATE
	rtc_data->update_cb = NULL;
	rtc_data->update_cb_data = NULL;
#endif /* RV8803_IRQ_GPIO_USE_UPDATE */

#endif /* RV8803_IRQ_GPIO_IN_USE */

	return 0;
}

/* RV8803 RTC driver API */
static const struct rtc_driver_api rv8803_rtc_driver_api = {
	.set_time = rv8803_rtc_set_time,
	.get_time = rv8803_rtc_get_time,
#if RV8803_IRQ_GPIO_USE_ALARM
	.alarm_get_supported_fields = rv8803_rtc_alarm_get_supported_fields,
	.alarm_set_time = rv8803_rtc_alarm_set_time,
	.alarm_get_time = rv8803_rtc_alarm_get_time,
	.alarm_is_pending = rv8803_rtc_alarm_is_pending,
	.alarm_set_callback = rv8803_rtc_alarm_set_callback,
#endif
#if RV8803_IRQ_GPIO_USE_UPDATE
	.update_set_callback = rv8803_update_set_callback,
#endif
};
#endif

#if CONFIG_RTC && CONFIG_RV8803_RTC_ENABLE
/* RV8803 RTC Initialization MACRO */
#define RV8803_RTC_INIT(n)                                                                         \
	static const struct rv8803_rtc_config rv8803_rtc_config_##n = {                            \
		.base_dev = DEVICE_DT_GET(DT_PARENT(DT_INST(n, DT_DRV_COMPAT))),                   \
	};                                                                                         \
	static struct rv8803_rtc_data rv8803_rtc_data_##n;                                         \
	DEVICE_DT_INST_DEFINE(n, rv8803_rtc_init, NULL, &rv8803_rtc_data_##n,                      \
			      &rv8803_rtc_config_##n, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY,       \
			      &rv8803_rtc_driver_api);
#endif

#if CONFIG_RTC && CONFIG_RV8803_RTC_ENABLE
/* Instanciate RV8803 RTC */
DT_INST_FOREACH_STATUS_OKAY(RV8803_RTC_INIT)
#endif
#undef DT_DRV_COMPAT
