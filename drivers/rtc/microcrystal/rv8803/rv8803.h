/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_RTC_RV8803_H_
#define ZEPHYR_DRIVERS_RTC_RV8803_H_

#include <zephyr/drivers/i2c.h>

/* Calendar Registers */
#define RV8803_REGISTER_SECONDS 0x00
#define RV8803_REGISTER_MINUTES 0x01
#define RV8803_REGISTER_HOURS   0x02
#define RV8803_REGISTER_WEEKDAY 0x03
#define RV8803_REGISTER_DATE    0x04
#define RV8803_REGISTER_MONTH   0x05
#define RV8803_REGISTER_YEAR    0x06

/* Alarm Registers */
#define RV8803_REGISTER_ALARM_MINUTES 0x08
#define RV8803_REGISTER_ALARM_HOURS   0x09
#define RV8803_REGISTER_ALARM_WADA    0x0A

/* Control Registers */
#define RV8803_REGISTER_EXTENSION 0x0D
#define RV8803_REGISTER_FLAG      0x0E
#define RV8803_REGISTER_CONTROL   0x0F

/* Structs */
/* RV8803 Base config */
struct rv8803_config {
	struct i2c_dt_spec i2c_bus;
};

/* RV8803 Base data */
struct rv8803_data {
};

#endif /* ZEPHYR_DRIVERS_RTC_RV8803_H_ */
