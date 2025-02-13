/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_RTC_H_
#define ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_RTC_H_

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

#define RV8803_EXTENSION_MASK_UPDATE (0x01 << 5)
#define RV8803_FLAG_MASK_UPDATE      (0x01 << 5)
#define RV8803_CONTROL_MASK_UPDATE   (0x01 << 5)

/* TM OFFSET */
#define RV8803_TM_MONTH 1

/* Control MACRO */
/* Check for partial incrementation when reads get 59 seconds */
#define RV8803_PARTIAL_SECONDS_INCR  59
#define RV8803_CORRECT_YEAR_LEAP_MIN (2000 - 1900) /* Diff between 2000 and tm base year 1900 */
#define RV8803_CORRECT_YEAR_LEAP_MAX (2099 - 1900) /* Diff between 2099 and tm base year 1900 */
#define RV8803_RESET_BIT             (0x01 << 0)
#define RV8803_ENABLE_ALARM          (0x01 << 3)
#define RV8803_DISABLE_ALARM         (0x00 << 3)
#define RV8803_WEEKDAY_ALARM         (0x00 << 6)
#define RV8803_MONTHDAY_ALARM        (0x01 << 6)

#define RV8803_UPDATE_PERIOD_SECOND (0x00 << 5)
#define RV8803_UPDATE_PERIOD_MINUTE (0x01 << 5)
#define RV8803_ENABLE_UPDATE        (0x01 << 5)
#define RV8803_DISABLE_UPDATE       (0x00 << 5)

/* Structs */
// clang-format off
#if defined(CONFIG_RTC_ALARM)
#if RV8803_HAS_IRQ
#define RV8803_RTC_IRQ_ALARM_AVAILABLE 1
#define RV8803_RTC_IRQ_ALARM(inst) DT_INST_ENUM_IDX(inst, alarm)
#define RV8803_RTC_IRQ_ALARM_HAS_PROP(inst) DT_INST_PROP(inst, use_irq)
#else
#define RV8803_RTC_IRQ_ALARM_AVAILABLE 0
#define RV8803_RTC_IRQ_ALARM(inst) 0
#define RV8803_RTC_IRQ_ALARM_HAS_PROP(inst) 0
#endif /* RV8803_HAS_IRQ */
#endif /* CONFIG_RTC_ALARM */

#if defined(CONFIG_RTC_UPDATE)
#if RV8803_HAS_IRQ
#define RV8803_RTC_IRQ_UPDATE_AVAILABLE 1
#define RV8803_RTC_IRQ_UPDATE(inst) DT_INST_ENUM_IDX(inst, update)
#define RV8803_RTC_IRQ_UPDATE_HAS_PROP(inst) DT_INST_PROP(inst, use_irq)
#else
#define RV8803_RTC_IRQ_UPDATE_AVAILABLE 0
#define RV8803_RTC_IRQ_UPDATE(inst) 0
#define RV8803_RTC_IRQ_UPDATE_HAS_PROP(inst) 0
#endif /* RV8803_HAS_IRQ */
#endif /* CONFIG_RTC_UPDATE */

#if RV8803_RTC_IRQ_ALARM_AVAILABLE || RV8803_RTC_IRQ_UPDATE_AVAILABLE
#define RV8803_RTC_IRQ_IN_USE 1
#define RV8803_RTC_IRQ_HAS_PROP(inst) DT_INST_PROP(inst, use_irq)
#else
#define RV8803_RTC_IRQ_IN_USE 0
#define RV8803_RTC_IRQ_HAS_PROP(inst) 0
#endif
// clang-format on

/* RV8803 RTC config */
struct rv8803_rtc_config {
	const struct device *mfd_dev;
	const bool irq_alarm;
	const bool irq_update;
};

#if RV8803_RTC_IRQ_IN_USE
// clang-format off
struct rv8803_rtc_irq {
	const struct device *dev;
	struct k_work work;
	int (*append_listener_fn)(const struct device *dev, struct k_work *worker);
};
// clang-format on
#endif /* RV8803_RTC_IRQ_IN_USE */

#if RV8803_RTC_IRQ_ALARM_AVAILABLE
struct rv8803_rtc_alarm {
	rtc_alarm_callback alarm_cb;
	void *alarm_cb_data;
};
#endif /* RV8803_RTC_IRQ_ALARM_AVAILABLE */

#if RV8803_RTC_IRQ_UPDATE_AVAILABLE
struct rv8803_rtc_update {
	rtc_update_callback update_cb;
	void *update_cb_data;
};
#endif /* RV8803_RTC_IRQ_UPDATE_AVAILABLE */

/* RV8803 RTC data */
struct rv8803_rtc_data {
	struct rv8803_rtc_irq *rtc_irq;
	struct rv8803_rtc_alarm *rtc_alarm;
	struct rv8803_rtc_update *rtc_update;
};
#endif /* ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_RTC_H_ */
