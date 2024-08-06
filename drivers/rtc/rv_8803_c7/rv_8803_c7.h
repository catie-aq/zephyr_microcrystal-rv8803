/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_RTC_MICROCRYSTAL_RV_8803_C7_H_
#define ZEPHYR_DRIVERS_RTC_MICROCRYSTAL_RV_8803_C7_H_

#include <zephyr/drivers/rtc.h>
#include <zephyr/drivers/i2c.h>

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

/* Control MACRO */
#define RV8803C7_PARTIAL_SECONDS_INCR 59 // Check for partial incrementation when reads get 59 seconds
#define RV8803C7_CORRECT_YEAR_LEAP_MIN (2000 - 1900) // Diff between 2000 and tm base year 1900
#define RV8803C7_CORRECT_YEAR_LEAP_MAX (2099 - 1900) // Diff between 2099 and tm base year 1900

/* Structs */
struct rv_8803_c7_config {
	struct i2c_dt_spec i2c_bus;
};

/* API */
static int rv_8803_c7_set_time(const struct device *dev, const struct rtc_time *timeptr);
static int rv_8803_c7_get_time(const struct device *dev, struct rtc_time *timeptr);

#endif /* ZEPHYR_DRIVERS_RTC_MICROCRYSTAL_RV_8803_C7_H_ */
