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
#else
#define RV8803_HAS_IRQ 0
#endif

#if RV8803_HAS_IRQ
#if CONFIG_RV8803_RTC_ENABLE
#if defined(CONFIG_RTC_ALARM)
#define RV8803_IRQ_GPIO_USE_ALARM 1
#endif /* CONFIG_RTC_ALARM */
#if defined(CONFIG_RTC_UPDATE)
#define RV8803_IRQ_GPIO_USE_UPDATE 1
#endif /* CONFIG_RTC_UPDATE */
#endif /* CONFIG_RV8803_RTC_ENABLE */
#if CONFIG_RV8803_COUNTER_ENABLE
#if defined(CONFIG_COUNTER)
#define RV8803_IRQ_GPIO_USE_COUNTER 1
#endif /* CONFIG_RTC_ALARM */
#endif /* RV8803_COUNTER_ENABLE */
#endif /* RV8803_HAS_IRQ */

#if defined(RV8803_IRQ_GPIO_USE_ALARM) || defined(RV8803_IRQ_GPIO_USE_UPDATE)
#define RV8803_IRQ_RTC_IN_USE 1
#endif
#if defined(RV8803_IRQ_GPIO_USE_COUNTER)
#define RV8803_IRQ_CNT_IN_USE 1
#endif
#if defined(RV8803_IRQ_RTC_IN_USE) || defined(RV8803_IRQ_CNT_IN_USE)
#define RV8803_IRQ_GPIO_IN_USE 1
#endif

/* Structs */
struct rv8803_config_irq {
#if RV8803_HAS_IRQ
	const struct gpio_dt_spec irq_gpio;
#endif
};

/* RV8803 Base config */
struct rv8803_config {
	struct i2c_dt_spec i2c_bus;
	struct rv8803_config_irq *gpio;
};

struct rv8803_battery {
#if CONFIG_RV8803_BATTERY_ENABLE
	bool power_on_reset;
	bool low_battery;
#endif /* CONFIG_RV8803_BATTERY_ENABLE */
};

struct rv8803_irq {
#if RV8803_HAS_IRQ
	struct gpio_callback gpio_cb;
#if RV8803_IRQ_RTC_IN_USE
	const struct device *rtc_dev;
	struct k_work rtc_work;
#endif /* RV8803_IRQ_RTC_IN_USE */
#if RV8803_IRQ_CNT_IN_USE
	const struct device *cnt_dev;
	struct k_work cnt_work;
#endif /* RV8803_IRQ_CNT_IN_USE */
#endif /* RV8803_HAS_IRQ */
};

/* RV8803 Base data */
struct rv8803_data {
	struct rv8803_battery *bat;
	struct rv8803_irq *irq;
};

#endif /* ZEPHYR_DRIVERS_RTC_RV8803_H_ */
