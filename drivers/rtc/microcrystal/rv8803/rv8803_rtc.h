/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_RTC_RV8803_RTC_H_
#define ZEPHYR_DRIVERS_RTC_RV8803_RTC_H_

/* Data masks */
#define RV8803_SECONDS_BITS GENMASK(6, 0)
#define RV8803_MINUTES_BITS GENMASK(6, 0)
#define RV8803_HOURS_BITS   GENMASK(5, 0)
#define RV8803_WEEKDAY_BITS GENMASK(6, 0)
#define RV8803_DATE_BITS    GENMASK(5, 0)
#define RV8803_MONTH_BITS   GENMASK(4, 0)
#define RV8803_YEAR_BITS    GENMASK(7, 0)

/* Registers Mask */
#define RV8803_EXTENSION_MASK_WADA   (0x01 << 6)
#define RV8803_FLAG_MASK_ALARM       (0x01 << 3)
#define RV8803_CONTROL_MASK_ALARM    (0x01 << 3)
#define RV8803_ALARM_ENABLE_MINUTES  (0x00 << 7)
#define RV8803_ALARM_ENABLE_HOURS    (0x00 << 7)
#define RV8803_ALARM_ENABLE_WADA     (0x00 << 7)
#define RV8803_ALARM_DISABLE_MINUTES (0x01 << 7)
#define RV8803_ALARM_DISABLE_HOURS   (0x01 << 7)
#define RV8803_ALARM_DISABLE_WADA    (0x01 << 7)
#define RV8803_ALARM_MASK_MINUTES    RV8803_ALARM_DISABLE_MINUTES
#define RV8803_ALARM_MASK_HOURS      RV8803_ALARM_DISABLE_HOURS
#define RV8803_ALARM_MASK_WADA       RV8803_ALARM_DISABLE_WADA

/* TM OFFSET */
#define RV8803_TM_MONTH 1

/* Control MACRO */
#define RV8803_PARTIAL_SECONDS_INCR  59 // Check for partial incrementation when reads get 59 seconds
#define RV8803_CORRECT_YEAR_LEAP_MIN (2000 - 1900) // Diff between 2000 and tm base year 1900
#define RV8803_CORRECT_YEAR_LEAP_MAX (2099 - 1900) // Diff between 2099 and tm base year 1900
#define RV8803_RESET_BIT             (0x01 << 0)
#define RV8803_ENABLE_ALARM          (0x01 << 3)
#define RV8803_DISABLE_ALARM         (0x00 << 3)
#define RV8803_WEEKDAY_ALARM         (0x00 << 6)
#define RV8803_MONTHDAY_ALARM        (0x01 << 6)

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(irq_gpios)
#if CONFIG_RV8803_RTC_ENABLE
#if defined(CONFIG_RTC_ALARM)
#define RV8803_IRQ_GPIO_USE_ALARM 1
#endif // CONFIG_RTC_ALARM
#if defined(CONFIG_RTC_UPDATE)
#define RV8803_IRQ_GPIO_USE_UPDATE 1
#endif // CONFIG_RTC_UPDATE
#endif // CONFIG_RV8803_RTC_ENABLE
#endif // DT_ANY_INST_HAS_PROP_STATUS_OKAY

#if defined(RV8803_IRQ_GPIO_USE_ALARM) || defined(RV8803_IRQ_GPIO_USE_UPDATE)
#define RV8803_IRQ_GPIO_IN_USE 1
#endif

/* Structs */
#if CONFIG_RTC && CONFIG_RV8803_RTC_ENABLE
/* RV8803 RTC config */
struct rv8803_rtc_config {
	const struct device *base_dev; // Parent device reference
#if RV8803_IRQ_GPIO_IN_USE
	const struct gpio_dt_spec irq_gpio;
#endif
};

/* RV8803 RTC data */
struct rv8803_rtc_data {
#if RV8803_IRQ_GPIO_IN_USE
	struct gpio_callback gpio_cb;
	const struct device *dev;
#endif

#if RV8803_IRQ_GPIO_USE_ALARM
	rtc_alarm_callback alarm_cb;
	void *alarm_cb_data;
	struct k_work alarm_work;
#endif
};
#endif

#endif /* ZEPHYR_DRIVERS_RTC_RV8803_RTC_H_ */