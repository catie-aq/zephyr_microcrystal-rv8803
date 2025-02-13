/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>

#include "rv8803_api.h"
#define DT_DRV_COMPAT microcrystal_rv8803_rtc
#include "rv8803_rtc.h"
#include <math.h>

LOG_MODULE_REGISTER(RV8803_RTC, CONFIG_RTC_LOG_LEVEL);

/**
 * @brief Set the current time for the RV8803 RTC.
 *
 * This function will set the current time for the RV8803 RTC device.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param timeptr Pointer to the rtc_time structure which contains the new time.
 *
 * @retval 0 If successful.
 * @retval -EINVAL If the timeptr is NULL or the year is not between 2000 and 2099.
 * @retval Negative errno code on other failure.
 */
static int rv8803_rtc_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	/* Valid date are between 2000 and 2099 */
	if ((timeptr == NULL) || (timeptr->tm_year < RV8803_RTC_CORRECT_YEAR_LEAP_MIN) ||
	    (timeptr->tm_year > RV8803_RTC_CORRECT_YEAR_LEAP_MAX)) {
		LOG_ERR("invalid time");
		return -EINVAL;
	}

	LOG_DBG("Set time: year[%u] month[%u] mday[%u] wday[%u] hours[%u] minutes[%u] seconds[%u]",
		timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday,
		timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);

	/* Init variables for i2c communication */
	const struct rv8803_rtc_config *rtc_config = dev->config;
	uint8_t regs[7];
	uint8_t control_reg[1];
	int err;

	regs[0] = bin2bcd(timeptr->tm_sec) & RV8803_RTC_SECONDS_BITS;
	regs[1] = bin2bcd(timeptr->tm_min) & RV8803_RTC_MINUTES_BITS;
	regs[2] = bin2bcd(timeptr->tm_hour) & RV8803_RTC_HOURS_BITS;
	regs[3] = (1 << timeptr->tm_wday) & RV8803_RTC_WEEKDAY_BITS;
	regs[4] = bin2bcd(timeptr->tm_mday) & RV8803_RTC_DATE_BITS;
	regs[5] = bin2bcd(timeptr->tm_mon + RV8803_TM_MONTH) & RV8803_RTC_MONTH_BITS;
	regs[6] =
		bin2bcd(timeptr->tm_year - RV8803_RTC_CORRECT_YEAR_LEAP_MIN) & RV8803_RTC_YEAR_BITS;

	/* Stopping time update clock */
	err = rv8803_bus_burst_read(rtc_config->mfd_dev, RV8803_REGISTER_CONTROL, control_reg,
				    sizeof(control_reg));
	if (err < 0) {
		return err;
	}
	control_reg[0] |= RV8803_RTC_RESET_BIT;
	err = rv8803_bus_burst_write(rtc_config->mfd_dev, RV8803_REGISTER_CONTROL, control_reg,
				     sizeof(control_reg));
	if (err < 0) {
		return err;
	}

	/* Write new time to RTC register */
	err = rv8803_bus_burst_write(rtc_config->mfd_dev, RV8803_REGISTER_SECONDS, regs,
				     sizeof(regs));
	if (err < 0) {
		return err;
	}

	/* Restart time update clock */
	control_reg[0] &= ~RV8803_RTC_RESET_BIT;
	return rv8803_bus_burst_write(rtc_config->mfd_dev, RV8803_REGISTER_CONTROL, control_reg,
				      sizeof(control_reg));
}

/**
 * @brief Retrieve the current time from the RV8803 RTC device.
 *
 * This function reads the current time from the RV8803 RTC registers and
 * populates the provided rtc_time structure with the retrieved values.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param timeptr Pointer to the rtc_time structure where the current time
 *                will be stored.
 *
 * @retval 0 If successful.
 * @retval -EINVAL If the timeptr is NULL.
 * @retval Negative errno code on other failure.
 */
static int rv8803_rtc_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	if (timeptr == NULL) {
		return -EINVAL;
	}

	/* Init variables for i2c communication */
	const struct rv8803_rtc_config *rtc_config = dev->config;
	uint8_t regs1[7];
	uint8_t regs2[7];
	uint8_t *correct = regs1;
	int err;

	err = rv8803_bus_burst_read(rtc_config->mfd_dev, RV8803_REGISTER_SECONDS, regs1,
				    sizeof(regs1));
	if (err < 0) {
		return err;
	}

	/* Check to confirm correct time */
	if ((regs1[0] & RV8803_RTC_SECONDS_BITS) == bin2bcd(RV8803_RTC_PARTIAL_SECONDS_INCR)) {
		err = rv8803_bus_burst_read(rtc_config->mfd_dev, RV8803_REGISTER_SECONDS, regs2,
					    sizeof(regs2));
		if (err < 0) {
			return err;
		}
		if ((regs2[0] & RV8803_RTC_SECONDS_BITS) !=
		    bin2bcd(RV8803_RTC_PARTIAL_SECONDS_INCR)) {
			correct = regs2;
		}
	}

	timeptr->tm_sec = bcd2bin(correct[0] & RV8803_RTC_SECONDS_BITS);
	timeptr->tm_min = bcd2bin(correct[1] & RV8803_RTC_MINUTES_BITS);
	timeptr->tm_hour = bcd2bin(correct[2] & RV8803_RTC_HOURS_BITS);
	timeptr->tm_wday = log2(correct[3] & RV8803_RTC_WEEKDAY_BITS);
	timeptr->tm_mday = bcd2bin(correct[4] & RV8803_RTC_DATE_BITS);
	timeptr->tm_mon = bcd2bin(correct[5] & RV8803_RTC_MONTH_BITS) - RV8803_TM_MONTH;
	timeptr->tm_year =
		bcd2bin(correct[6] & RV8803_RTC_YEAR_BITS) + RV8803_RTC_CORRECT_YEAR_LEAP_MIN;

	/* Unused */
	timeptr->tm_nsec = 0;
	timeptr->tm_isdst = -1;
	timeptr->tm_yday = -1;

	LOG_DBG("Get time: year[%u] month[%u] mday[%u] wday[%u] hours[%u] minutes[%u] seconds[%u]",
		timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday,
		timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);
	return 0;
}

#if RV8803_RTC_IRQ_IN_USE
/**
 * @brief Handle the RTC interrupt worker.
 *
 * This function is called by the k_work framework when the RTC
 * interrupt is triggered. It reads the FLAG register and checks for the ALARM
 * and UPDATE flags. If the ALARM flag is set, it calls the alarm callback
 * function. If the UPDATE flag is set, it calls the update callback function.
 *
 * @param p_work Pointer to the k_work structure containing the work item.
 */
static void rv8803_rtc_worker(struct k_work *p_work)
{
	struct rv8803_rtc_irq *data = CONTAINER_OF(p_work, struct rv8803_rtc_irq, work);
	const struct rv8803_rtc_config *rtc_config = data->dev->config;
	const struct rv8803_rtc_data *rtc_data = data->dev->data;

	uint8_t reg;
	int err;

	LOG_DBG("Process RTC worker from interrupt");

	err = rv8803_bus_reg_read_byte(rtc_config->mfd_dev, RV8803_REGISTER_FLAG, &reg);
	if (err < 0) {
		LOG_ERR("RTC worker I2C read FLAGS error");
	}

#if CONFIG_RTC_ALARM
	if (reg & RV8803_FLAG_MASK_ALARM) {
		LOG_DBG("FLAG Alarm");
		if (rtc_data->rtc_alarm != NULL && rtc_data->rtc_alarm->alarm_cb != NULL) {
			LOG_DBG("Calling Alarm callback");
			rtc_data->rtc_alarm->alarm_cb(data->dev, 0,
						      rtc_data->rtc_alarm->alarm_cb_data);

			err = rv8803_bus_reg_update_byte(rtc_config->mfd_dev, RV8803_REGISTER_FLAG,
							 RV8803_FLAG_MASK_ALARM,
							 RV8803_RTC_DISABLE_ALARM);
			if (err < 0) {
				LOG_ERR("GPIO worker I2C update ALARM FLAG error");
			}
		}
	}
#endif /* CONFIG_RTC_ALARM */

#if CONFIG_RTC_UPDATE
	if (reg & RV8803_FLAG_MASK_UPDATE) {
		LOG_DBG("FLAG Update");
		if (rtc_data->rtc_update != NULL && rtc_data->rtc_update->update_cb != NULL) {
			LOG_DBG("Calling Update callback");
			rtc_data->rtc_update->update_cb(data->dev,
							rtc_data->rtc_update->update_cb_data);

			err = rv8803_bus_reg_update_byte(rtc_config->mfd_dev, RV8803_REGISTER_FLAG,
							 RV8803_FLAG_MASK_UPDATE,
							 RV8803_RTC_DISABLE_UPDATE);
			if (err < 0) {
				LOG_ERR("GPIO worker I2C update UPDATE FLAG error");
			}
		}
	}
#endif /* CONFIG_RTC_UPDATE */
}
#endif /* RV8803_RTC_IRQ_IN_USE */

#if CONFIG_RTC_ALARM
/**
 * @brief Validates the RTC alarm time and mask settings.
 *
 * This function checks the validity of the provided RTC alarm time and mask
 * settings. It ensures that the seconds value is zero and validates the
 * minute, hour, month day, and weekday fields based on the given mask.
 * The function also checks for mutually exclusive settings of month day
 * and weekday in the mask.
 *
 * @param timeptr Pointer to an rtc_time structure containing the alarm time to be validated.
 * @param mask Bitmask indicating which fields of the time structure are being used.
 *
 * @retval true If the alarm time and mask settings are valid.
 * @retval false If any of the time fields or mask settings are invalid.
 */

static bool rv8803_rtc_alarm_time_valid(const struct rtc_time *timeptr, uint16_t mask)
{
	if ((mask & RTC_ALARM_TIME_MASK_MINUTE) && (timeptr->tm_min < 0 || timeptr->tm_min > 59)) {
		LOG_ERR("MINUTE: 0 <= [%d] <= 59", timeptr->tm_min);
		return false;
	}

	if ((mask & RTC_ALARM_TIME_MASK_HOUR) && (timeptr->tm_hour < 0 || timeptr->tm_hour > 23)) {
		LOG_ERR("HOUR: 0 <= [%d] <= 23", timeptr->tm_hour);
		return false;
	}

	if ((mask & RTC_ALARM_TIME_MASK_MONTHDAY) && (mask & RTC_ALARM_TIME_MASK_WEEKDAY)) {
		LOG_ERR("WEEKDAY & MONTHDAY are incompatible");
		return false;
	}

	if ((mask & RTC_ALARM_TIME_MASK_MONTHDAY) &&
	    (timeptr->tm_mday < 1 || timeptr->tm_mday > 31)) {
		LOG_ERR("MDAY: 1 <= [%d] <= 31", timeptr->tm_mday);
		return false;
	}

	if ((mask & RTC_ALARM_TIME_MASK_WEEKDAY) &&
	    (timeptr->tm_wday < 0 || timeptr->tm_wday > 6)) {
		LOG_ERR("WDAY: 0 <= [%d] <= 6", timeptr->tm_wday);
		return false;
	}

	return true;
}

/**
 * @brief Gets the supported fields for the RV8803 RTC alarm.
 *
 * This function returns a mask of supported fields for the alarm time on the RV8803
 * RTC. The returned mask is a combination of the following values:
 *
 * - RTC_ALARM_TIME_MASK_MINUTE
 * - RTC_ALARM_TIME_MASK_HOUR
 * - RTC_ALARM_TIME_MASK_MONTHDAY
 * - RTC_ALARM_TIME_MASK_WEEKDAY
 *
 * The returned mask is used to determine which fields of the @a timeptr structure
 * are valid when calling rv8803_rtc_alarm_set().
 *
 * @param dev Pointer to the device structure.
 * @param id Unused.
 * @param mask Pointer to a uint16_t that receives the supported fields mask.
 *
 * @retval 0 If successful.
 */
static int rv8803_rtc_alarm_get_supported_fields(const struct device *dev, uint16_t id,
						 uint16_t *mask)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(id);

	(*mask) = (RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |
		   RTC_ALARM_TIME_MASK_MONTHDAY | RTC_ALARM_TIME_MASK_WEEKDAY);

	return 0;
}

/**
 * @brief Set the alarm time for the RV8803 RTC.
 *
 * This function configures the alarm time on the RV8803 RTC using the provided
 * time structure and mask. It validates the time and mask, updates the RTC registers
 * accordingly, and activates or deactivates the alarm interrupt based on the mask.
 *
 * @note This function activate physical interruption only when alarm IRQ is available.
 *
 * @param dev Pointer to the device structure.
 * @param id Unused.
 * @param mask Mask specifying which fields of the time structure are valid for the alarm.
 *             A mask value of 0 removes the alarm interrupt.
 * @param timeptr Pointer to a rtc_time structure containing the desired alarm time.
 *                It must be non-NULL if the mask is non-zero.
 *
 * @retval 0 If successful.
 * @retval -EINVAL If the time pointer is NULL when the mask is non-zero, or if the time
 *                 or mask is invalid.
 * @retval Negative errno code on failure to update RTC registers.
 */

static int rv8803_rtc_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				     const struct rtc_time *timeptr)
{
	ARG_UNUSED(id);
	const struct rv8803_rtc_config *rtc_config = dev->config;
	struct rv8803_rtc_data *data = dev->data;
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
		err = rv8803_bus_reg_update_byte(rtc_config->mfd_dev, RV8803_REGISTER_CONTROL,
						 RV8803_CONTROL_MASK_ALARM,
						 RV8803_RTC_DISABLE_ALARM);
		if (err < 0) {
			LOG_ERR("Update CONTROL: [%d]", err);
			return err;
		}
		err = rv8803_bus_reg_update_byte(rtc_config->mfd_dev, RV8803_REGISTER_FLAG,
						 RV8803_FLAG_MASK_ALARM, RV8803_RTC_DISABLE_ALARM);
		if (err < 0) {
			LOG_ERR("Update FLAG: [%d]", err);
			return err;
		}

		return 0;
	}

	/* AIE and AF to 0 -> stop interrupt and clear interrupt flags */
	err = rv8803_bus_reg_update_byte(rtc_config->mfd_dev, RV8803_REGISTER_CONTROL,
					 RV8803_CONTROL_MASK_ALARM, RV8803_RTC_DISABLE_ALARM);
	if (err < 0) {
		LOG_ERR("Update CONTROL: [%d]", err);
		return err;
	}
	err = rv8803_bus_reg_update_byte(rtc_config->mfd_dev, RV8803_REGISTER_FLAG,
					 RV8803_FLAG_MASK_ALARM, RV8803_RTC_DISABLE_ALARM);
	if (err < 0) {
		LOG_ERR("Update FLAG: [%d]", err);
		return err;
	}

	/* Set WADA to 0 or 1 */
	uint8_t wada = RV8803_RTC_WEEKDAY_ALARM;
	if (mask && RTC_ALARM_TIME_MASK_MONTHDAY) {
		wada = RV8803_RTC_MONTHDAY_ALARM;
	}
	err = rv8803_bus_reg_update_byte(rtc_config->mfd_dev, RV8803_REGISTER_EXTENSION,
					 RV8803_EXTENSION_MASK_WADA, wada);
	if (err < 0) {
		LOG_ERR("Update EXTENSION: [%d]", err);
		return err;
	}

	/* Set desired time and alarm */
	uint8_t regs[3];
	if (mask & RTC_ALARM_TIME_MASK_MINUTE) {
		regs[0] = RV8803_RTC_ALARM_ENABLE_MINUTES;
		regs[0] |= bin2bcd(timeptr->tm_min) & RV8803_RTC_MINUTES_BITS;
	} else {
		regs[0] = RV8803_RTC_ALARM_DISABLE_MINUTES;
	}

	if (mask & RTC_ALARM_TIME_MASK_HOUR) {
		regs[1] = RV8803_RTC_ALARM_ENABLE_HOURS;
		regs[1] |= bin2bcd(timeptr->tm_hour) & RV8803_RTC_HOURS_BITS;
	} else {
		regs[1] = RV8803_RTC_ALARM_DISABLE_HOURS;
	}

	if (mask & RTC_ALARM_TIME_MASK_WEEKDAY) {
		regs[2] = RV8803_RTC_ALARM_ENABLE_WADA;
		regs[2] |= (1 << timeptr->tm_wday) & RV8803_RTC_WEEKDAY_BITS;
	} else if (mask & RTC_ALARM_TIME_MASK_MONTHDAY) {
		regs[2] = RV8803_RTC_ALARM_ENABLE_WADA;
		regs[2] |= bin2bcd(timeptr->tm_mday) & RV8803_RTC_DATE_BITS;
	} else {
		regs[2] = RV8803_RTC_ALARM_DISABLE_WADA;
	}

	err = rv8803_bus_burst_write(rtc_config->mfd_dev, RV8803_RTC_ALARM_REGISTER_MINUTES, regs,
				     sizeof(regs));
	if (err < 0) {
		LOG_ERR("Write ALARM: [%d]", err);
		return err;
	}

	/* AIE 1 -> activate interrupt */
	if (data->rtc_alarm != NULL) {
		err = rv8803_bus_reg_update_byte(rtc_config->mfd_dev, RV8803_REGISTER_CONTROL,
						 RV8803_CONTROL_MASK_ALARM,
						 RV8803_RTC_ENABLE_ALARM);
		if (err < 0) {
			LOG_ERR("Update CONTROL: [%d]", err);
			return err;
		}
	}

	return 0;
}

/**
 * @brief Retrieves the current alarm settings for the RTC device.
 *
 * This function reads the current alarm settings from the RTC device and fills
 * in the provided timeptr structure with the alarm time. The mask parameter is
 * also populated with the supported fields of the alarm time.
 *
 * @param dev Pointer to the device structure.
 * @param id Unused.
 * @param mask Pointer to a uint16_t that will be populated with the supported
 *             fields of the alarm time.
 * @param timeptr Pointer to an rtc_time structure that will be populated with the
 *                alarm time.
 *
 * @retval 0 If successful.
 * @retval -EINVAL If the time pointer is NULL.
 * @retval Negative errno code on other failures.
 */
static int rv8803_rtc_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				     struct rtc_time *timeptr)
{
	ARG_UNUSED(id);
	const struct rv8803_rtc_config *rtc_config = dev->config;
	int err;

	if (timeptr == NULL) {
		return -EINVAL;
	}
	(*mask) = 0;

	uint8_t regs[3];
	err = rv8803_bus_burst_read(rtc_config->mfd_dev, RV8803_RTC_ALARM_REGISTER_MINUTES, regs,
				    sizeof(regs));
	if (err < 0) {
		return err;
	}

	if ((regs[0] & RV8803_RTC_ALARM_MASK_MINUTES) == RV8803_RTC_ALARM_ENABLE_MINUTES) {
		(*mask) |= RTC_ALARM_TIME_MASK_MINUTE;
		timeptr->tm_min = bcd2bin(regs[0] & RV8803_RTC_MINUTES_BITS);
	}

	if ((regs[1] & RV8803_RTC_ALARM_MASK_HOURS) == RV8803_RTC_ALARM_ENABLE_HOURS) {
		(*mask) |= RTC_ALARM_TIME_MASK_HOUR;
		timeptr->tm_hour = bcd2bin(regs[1] & RV8803_RTC_HOURS_BITS);
	}

	if ((regs[2] & RV8803_RTC_ALARM_MASK_WADA) == RV8803_RTC_ALARM_ENABLE_WADA) {
		uint8_t wada;
		err = rv8803_bus_reg_read_byte(rtc_config->mfd_dev, RV8803_REGISTER_EXTENSION,
					       &wada);
		if (err < 0) {
			return err;
		}

		if ((wada & RV8803_EXTENSION_MASK_WADA) == RV8803_RTC_WEEKDAY_ALARM) {
			(*mask) |= RTC_ALARM_TIME_MASK_WEEKDAY;
			timeptr->tm_wday = log2(regs[2] & RV8803_RTC_WEEKDAY_BITS);
		} else {
			(*mask) |= RTC_ALARM_TIME_MASK_MONTHDAY;
			timeptr->tm_mday = bcd2bin(regs[2] & RV8803_RTC_DATE_BITS);
		}
	}

	return 0;
}

/**
 * @brief Checks if the alarm is pending.
 *
 * This function checks if the alarm is pending and if so, clears the alarm flag.
 *
 * @param dev Pointer to the device structure.
 * @param id Alarm ID. Currently only one alarm is supported, so this parameter is ignored.
 *
 * @return 1 if the alarm is pending, 0 if it is not, negative errno on failure.
 */
static int rv8803_rtc_alarm_is_pending(const struct device *dev, uint16_t id)
{
	ARG_UNUSED(id);
	const struct rv8803_rtc_config *rtc_config = dev->config;
	uint8_t reg;
	int err;

	err = rv8803_bus_reg_read_byte(rtc_config->mfd_dev, RV8803_REGISTER_FLAG, &reg);
	if (err < 0) {
		return err;
	}

	if (reg & RV8803_FLAG_MASK_ALARM) {
		err = rv8803_bus_reg_update_byte(rtc_config->mfd_dev, RV8803_REGISTER_FLAG,
						 RV8803_FLAG_MASK_ALARM, RV8803_RTC_DISABLE_ALARM);
		if (err < 0) {
			return err;
		}

		return 1;
	}

	return 0;
}

#if RV8803_RTC_IRQ_ALARM_AVAILABLE
/**
 * @brief Set the callback function for the RTC alarm interrupt.
 *
 * This function sets the callback function that will be called when the RTC alarm
 * interrupt occurs. The callback function is called with the user-provided data
 * as the only argument.
 *
 * @note This function is only available if the RTC alarm interrupt is available.
 *
 * @param dev Pointer to the device structure.
 * @param id Alarm ID. Currently only one alarm is supported, so this parameter is ignored.
 * @param callback Callback function to set.
 * @param user_data User-provided data that is passed as the only argument to the callback function.
 *
 * @return 0 on success, negative errno code on failure.
 */
static int rv8803_rtc_alarm_set_callback(const struct device *dev, uint16_t id,
					 rtc_alarm_callback callback, void *user_data)
{
	ARG_UNUSED(id);

	const struct rv8803_rtc_config *rtc_config = dev->config;
	if (!rtc_config->irq_alarm) {
		return -ENOTSUP;
	}

	if (!rv8803_irq_gpio_is_available(rtc_config->mfd_dev)) {
		LOG_ERR("GPIO IRQ is not supported");
		return -ENOTSUP;
	}

	struct rv8803_rtc_data *data = dev->data;
	data->rtc_alarm->alarm_cb = callback;
	data->rtc_alarm->alarm_cb_data = user_data;

	return 0;
}
#endif /* RV8803_RTC_IRQ_ALARM_AVAILABLE */
#endif /* CONFIG_RTC_ALARM */

#if CONFIG_RTC_UPDATE
/**
 * @brief Setup or disable the RTC update interrupt.
 *
 * This function sets or clears the UIE bit in the control register and
 * the UF bit in the flag register. If the disable parameter is true,
 * the bits are cleared. If the disable parameter is false, the UIE bit
 * is set, and the USEL bit in the extension register is cleared.
 *
 * @note This function is only available if the RTC alarm interrupt is available.
 *
 * @param dev Pointer to the device structure.
 * @param disable Set to true to disable the update interrupt, false to enable it.
 *
 * @retval 0 on success.
 * @retval Negative errno code on failure.
 */
static int rv8803_setup_update_interrupt(const struct device *dev)
{
	const struct rv8803_rtc_config *rtc_config = dev->config;
	int err;

	/* UIE and UF to 0 : stop interrupt */
	err = rv8803_bus_reg_update_byte(rtc_config->mfd_dev, RV8803_REGISTER_CONTROL,
					 RV8803_CONTROL_MASK_UPDATE, RV8803_RTC_DISABLE_UPDATE);
	if (err < 0) {
		return err;
	}
	err = rv8803_bus_reg_update_byte(rtc_config->mfd_dev, RV8803_REGISTER_FLAG,
					 RV8803_FLAG_MASK_UPDATE, RV8803_RTC_DISABLE_UPDATE);
	if (err < 0) {
		return err;
	}

	/* Choose USEL value */
	err = rv8803_bus_reg_update_byte(rtc_config->mfd_dev, RV8803_REGISTER_EXTENSION,
					 RV8803_EXTENSION_MASK_UPDATE, RV8803_RTC_DISABLE_UPDATE);
	if (err < 0) {
		return err;
	}

	/* UIE to 1 : start interrupt */
	err = rv8803_bus_reg_update_byte(rtc_config->mfd_dev, RV8803_REGISTER_CONTROL,
					 RV8803_CONTROL_MASK_UPDATE, RV8803_RTC_ENABLE_UPDATE);
	if (err < 0) {
		return err;
	}

	return 0;
}

#if RV8803_RTC_IRQ_UPDATE_AVAILABLE
/**
 * @brief Sets the callback function for the RTC update interrupt.
 *
 * This function configures the callback and user data for the RTC update
 * interrupt. The callback will be invoked when the update event occurs.
 * If the callback is NULL and user_data is not NULL, the function returns
 * an error.
 *
 * @param dev Pointer to the device structure.
 * @param callback The function to be called when the update interrupt occurs.
 * @param user_data The user data to be passed to the callback function.
 *
 * @retval 0 on success.
 * @retval -ENOTSUP if the GPIO IRQ is not supported.
 * @retval -EINVAL if the callback is NULL and user_data is not NULL.
 */
static int rv8803_update_set_callback(const struct device *dev, rtc_update_callback callback,
				      void *user_data)
{
	const struct rv8803_rtc_config *rtc_config = dev->config;
	if (!rtc_config->irq_update) {
		return -ENOTSUP;
	}

	if (!rv8803_irq_gpio_is_available(rtc_config->mfd_dev)) {
		LOG_ERR("GPIO IRQ is not supported");
		return -ENOTSUP;
	}

	struct rv8803_rtc_data *data = dev->data;
	data->rtc_update->update_cb = callback;
	data->rtc_update->update_cb_data = user_data;

	if ((callback == NULL) && (user_data != NULL)) {
		LOG_ERR("callback is NULL and user_data is not NULL");
		return -EINVAL;
	}

	return 0;
}
#endif /* RV8803_RTC_IRQ_UPDATE_AVAILABLE */
#endif /* CONFIG_RTC_UPDATE */

/**
 * @brief Initializes the RV8803 RTC driver module.
 *
 * This function initializes the RV8803 RTC driver module. It checks if the
 * parent device is ready and if the append_listener_fn is defined. If the
 * append_listener_fn is defined, it appends the listener to the parent device.
 * It also sets up or disables the update interrupt based on the presence of
 * the CONFIG_RTC_UPDATE macro.
 *
 * @param dev Pointer to the device structure.
 *
 * @retval 0 on success.
 * @retval Negative errno code on failure.
 */
static int rv8803_rtc_init(const struct device *dev)
{
	const struct rv8803_rtc_config *rtc_config = dev->config;

	if (!device_is_ready(rtc_config->mfd_dev)) {
		return -ENODEV;
	}

#if RV8803_RTC_IRQ_IN_USE
	struct rv8803_rtc_data *rtc_data = dev->data;

	/**
	 * This block prevents the compiler from warning about unused functions
	 * when the corresponding features are not enabled.
	 */
	if (rtc_data->rtc_irq == NULL) {
		(void)rv8803_rtc_worker;
	} else if (rtc_data->rtc_irq->work.handler == NULL) {
		(void)rv8803_rtc_worker;
	}

	if ((rtc_data->rtc_irq != NULL) && (rtc_data->rtc_irq->append_listener_fn != NULL)) {
		LOG_DBG("Append listener");
		int err = rtc_data->rtc_irq->append_listener_fn(rtc_config->mfd_dev,
								&rtc_data->rtc_irq->work);
		if (err < 0) {
			LOG_ERR("Listener append error [%d]", err);
		}
	}
#endif /* RV8803_RTC_IRQ_IN_USE */

#if CONFIG_RTC_UPDATE
	int err = rv8803_setup_update_interrupt(dev);
	if (err < 0) {
		return err;
	}
#endif

	LOG_INF("RV8803 RTC INIT");

	return 0;
}

/**
 * @brief RV8803 RTC driver API structure.
 *
 * @details This structure contains the following:
 *
 * - @a set_time: Set the time of the RTC.
 * - @a get_time: Get the current time of the RTC.
 * - @a alarm_get_supported_fields: Get the supported fields for the alarm.
 * - @a alarm_set_time: Set the time of the alarm.
 * - @a alarm_get_time: Get the current time of the alarm.
 * - @a alarm_is_pending: Check if the alarm is pending.
 * - @a alarm_set_callback: Set the callback function for the alarm.
 * - @a update_set_callback: Set the callback function for the update.
 */
static const struct rtc_driver_api rv8803_rtc_driver_api = {
	.set_time = rv8803_rtc_set_time,
	.get_time = rv8803_rtc_get_time,
#if CONFIG_RTC_ALARM
	.alarm_get_supported_fields = rv8803_rtc_alarm_get_supported_fields,
	.alarm_set_time = rv8803_rtc_alarm_set_time,
	.alarm_get_time = rv8803_rtc_alarm_get_time,
	.alarm_is_pending = rv8803_rtc_alarm_is_pending,
#if RV8803_RTC_IRQ_ALARM_AVAILABLE
	.alarm_set_callback = rv8803_rtc_alarm_set_callback,
#endif /* RV8803_RTC_IRQ_ALARM_AVAILABLE */
#endif /* CONFIG_RTC_ALARM */
#if RV8803_RTC_IRQ_UPDATE_AVAILABLE
	.update_set_callback = rv8803_update_set_callback,
#endif /* RV8803_RTC_IRQ_UPDATE_AVAILABLE */
};

/**
 * @brief Check Devicetree configuration for RV8803 RTC
 *
 * This macro performs compile-time checks to ensure that the Devicetree
 * configuration is correct for the RV8803 RTC device. It verifies that if
 * the alarm or update IRQs are enabled, the 'use-irq' property is also set.
 *
 * @param n The instance number
 */
#define RV8803_RTC_DT_CHECK(n)                                                                     \
	BUILD_ASSERT(!RV8803_RTC_IRQ_ALARM(n) || RV8803_RTC_IRQ_ALARM_HAS_PROP(n),                 \
		     "Devicetree ERROR microcrystal,rv8803-rtc uses alarm = irq; but not "         \
		     "use-irq;. Did you forget to add use-irq;");                                  \
	BUILD_ASSERT(!RV8803_RTC_IRQ_UPDATE(n) || RV8803_RTC_IRQ_UPDATE_HAS_PROP(n),               \
		     "Devicetree ERROR microcrystal,rv8803-rtc uses update = irq; but not "        \
		     "use-irq;. Did you forget to add use-irq;");

/**
 * @brief Macro to initialize the RTC irq struct
 *
 * This macro initializes the rv8803_rtc_irq struct with the handler and
 * append_listener_fn pointers. If the IRQ is used for alarm or update, the
 * appropriate handler is set.
 *
 * @param n The instance number
 */
#define RV8803_RTC_IRQ_INIT(n)                                                                     \
	IF_ENABLED(RV8803_RTC_IRQ_HAS_PROP(n),                                                     \
		   (static struct rv8803_rtc_irq rv8803_rtc_irq_##n = {                            \
			    .dev = DEVICE_DT_GET(DT_DRV_INST(n)),                                  \
			    COND_CODE_1(RV8803_RTC_IRQ_ALARM(n),                                   \
					(.work.handler = rv8803_rtc_worker,                        \
					 .append_listener_fn = rv8803_append_irq_listener, ),      \
					(COND_CODE_1(RV8803_RTC_IRQ_UPDATE(n),                     \
						     (.work.handler = rv8803_rtc_worker,           \
						      .append_listener_fn =                        \
							      rv8803_append_irq_listener, ),       \
						     (.work.handler = NULL,                        \
						      .append_listener_fn = NULL, ))))};))

/**
 * @brief Macro to initialize the rv8803_rtc_alarm struct
 *
 * This macro initializes the rv8803_rtc_alarm struct with the alarm callback
 * and data pointers.
 *
 * @param n The instance number
 */
#define RV8803_RTC_ALARM_INIT(n)                                                                   \
	IF_ENABLED(RV8803_RTC_IRQ_ALARM(n),                                                        \
		   (static struct rv8803_rtc_alarm rv8803_rtc_alarm_##n = {                        \
			    .alarm_cb = NULL,                                                      \
			    .alarm_cb_data = NULL,                                                 \
		    };))

/**
 * @brief Macro to initialize the rv8803_rtc_update struct
 *
 * This macro initializes the rv8803_rtc_update struct with the update callback
 * and data pointers.
 *
 * @param n The instance number
 */
#define RV8803_RTC_UPDATE_INIT(n)                                                                  \
	IF_ENABLED(RV8803_RTC_IRQ_UPDATE(n),                                                       \
		   (static struct rv8803_rtc_update rv8803_rtc_update_##n = {                      \
			    .update_cb = NULL,                                                     \
			    .update_cb_data = NULL,                                                \
		    };))

/**
 * @brief Macro to initialize the RV8803 RTC config struct
 *
 * This macro initializes the rv8803_rtc_config struct with the parent
 * device reference obtained from the Devicetree.
 *
 * @param n The instance number
 */
#define RV8803_RTC_CONFIG_INIT(n)                                                                  \
	static const struct rv8803_rtc_config rv8803_rtc_config_##n = {                            \
		.mfd_dev = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(n))),                               \
		.irq_alarm = RV8803_RTC_IRQ_ALARM(n),                                              \
		.irq_update = RV8803_RTC_IRQ_UPDATE(n),                                            \
	};

/**
 * @brief Macro to initialize the RV8803 RTC data struct
 *
 * This macro initializes the rv8803_rtc_data struct with the appropriate
 * pointers to the rv8803_rtc_irq, rv8803_rtc_alarm, and rv8803_rtc_update
 * structs if the corresponding IRQs are enabled in the Devicetree.
 *
 * @param n The instance number
 */
#define RV8803_RTC_DATA_INIT(n)                                                                    \
	static struct rv8803_rtc_data rv8803_rtc_data_##n = {                                      \
		COND_CODE_1(RV8803_RTC_IRQ_HAS_PROP(n), (.rtc_irq = &rv8803_rtc_irq_##n, ),        \
			    (.rtc_irq = NULL, ))                                                   \
			COND_CODE_1(RV8803_RTC_IRQ_ALARM(n),                                       \
				    (.rtc_alarm = &rv8803_rtc_alarm_##n, ), (.rtc_alarm = NULL, )) \
				COND_CODE_1(RV8803_RTC_IRQ_UPDATE(n),                              \
					    (.rtc_update = &rv8803_rtc_update_##n, ),              \
					    (.rtc_update = NULL, ))};

/**
 * @brief Macro to generate the RV8803 RTC instances
 *
 * This macro is used to generate the RV8803 RTC instances. It is used to
 * iterate over all instances of the RV8803 RTC in the Devicetree. For each
 * instance, the macro initializes the rv8803_rtc_config, rv8803_rtc_data,
 * rv8803_rtc_irq, rv8803_rtc_alarm, and rv8803_rtc_update structs with the
 * appropriate values, and registers the device with the kernel using the
 * DEVICE_DT_INST_DEFINE macro.
 *
 * @param n The instance number
 */
#define RV8803_RTC_INIT(n)                                                                         \
	RV8803_RTC_DT_CHECK(n)                                                                     \
	RV8803_RTC_IRQ_INIT(n)                                                                     \
	RV8803_RTC_ALARM_INIT(n)                                                                   \
	RV8803_RTC_UPDATE_INIT(n)                                                                  \
	RV8803_RTC_CONFIG_INIT(n)                                                                  \
	RV8803_RTC_DATA_INIT(n)                                                                    \
	DEVICE_DT_INST_DEFINE(n, rv8803_rtc_init, NULL, &rv8803_rtc_data_##n,                      \
			      &rv8803_rtc_config_##n, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY,       \
			      &rv8803_rtc_driver_api);

/**
 * @brief Generate RV8803 RTC instances
 *
 * This macro is used to generate the RV8803 RTC instances. It is used to
 * iterate over all instances of the RV8803 RTC in the Devicetree. For each
 * instance, the RV8803_RTC_INIT macro is called to initialize the instance.
 *
 * @details The macro DT_INST_FOREACH_STATUS_OKAY is used to iterate over all
 * instances of the RV8803 RTC in the Devicetree. For each instance,
 * the RV8803_RTC_INIT macro is called to initialize the structs and register
 * the device.
 */
DT_INST_FOREACH_STATUS_OKAY(RV8803_RTC_INIT)
#undef DT_DRV_COMPAT
