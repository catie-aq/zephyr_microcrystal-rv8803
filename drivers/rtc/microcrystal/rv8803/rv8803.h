/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_RTC_RV8803_H_
#define ZEPHYR_DRIVERS_RTC_RV8803_H_

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>

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

/* Low Voltage Flag */
#define RV8803_FLAG_MASK_LOW_VOLTAGE_1 (0x01 << 0)
#define RV8803_FLAG_MASK_LOW_VOLTAGE_2 (0x01 << 1)

/* Timing constraint */
#define RV8803_STARTUP_TIMING_MS 80

/* DEFINITION of PROPERTY PARENT cf. include/zephyr/devicetree.h */
#define DT_INST_PARENT_NODE_HAS_PROP(inst, prop) DT_NODE_HAS_PROP(DT_INST_PARENT(inst), prop)

#define DT_ANY_INST_PARENT_HAS_PROP_STATUS_OKAY__(idx, prop)                                       \
	COND_CODE_1(DT_INST_PARENT_NODE_HAS_PROP(idx, prop), (1, ), ())

#define DT_ANY_INST_PARENT_HAS_PROP_STATUS_OKAY_(prop)                                             \
	DT_INST_FOREACH_STATUS_OKAY_VARGS(DT_ANY_INST_PARENT_HAS_PROP_STATUS_OKAY__, prop)

#define DT_ANY_INST_PARENT_HAS_PROP_STATUS_OKAY(prop)                                              \
	COND_CODE_1(IS_EMPTY(DT_ANY_INST_PARENT_HAS_PROP_STATUS_OKAY_(prop)), (0), (1))

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(irq_gpios)
#define RV8803_HAS_IRQ 1
#elif DT_ANY_INST_PARENT_HAS_PROP_STATUS_OKAY(irq_gpios)
#define RV8803_HAS_IRQ 1
#endif

/* Structs */
/* RV8803 Base config */
struct rv8803_config {
	struct i2c_dt_spec i2c_bus;
#if RV8803_HAS_IRQ
	const struct gpio_dt_spec irq_gpio;
#endif
};

/* RV8803 Base data */
struct rv8803_data {
#if RV8803_HAS_IRQ
	const struct device *rtc_dev;
	const struct device *cnt_dev;
	struct gpio_callback gpio_cb;
	struct k_work rtc_work;
	struct k_work cnt_work;
#endif
#if CONFIG_RV8803_DETECT_BATTERY_STATE
	bool power_on_reset;
	bool low_battery;
#endif /* CONFIG_RV8803_DETECT_BATTERY_STATE */
};

#endif /* ZEPHYR_DRIVERS_RTC_RV8803_H_ */
