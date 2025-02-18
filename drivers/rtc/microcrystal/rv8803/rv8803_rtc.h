/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_RTC_H_
#define ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_RTC_H_

#include <zephyr/drivers/rtc.h>

/**
 * @brief Time month offset
 *
 * The month value is offset by 1. For example, January is represented by 1,
 * February by 2, and so on.
 */
#define RV8803_TM_MONTH 1

/**
 * @brief Macros for RV8803 RTC alarm registers
 *
 * These macros are used to access the RV8803 RTC alarm registers and to
 * enable or disable the alarm.
 *
 * @details
 * The macros are used to set the alarm time on the RTC, and to enable or
 * disable the alarm interrupt.
 *
 * RV8803_RTC_ALARM_REGISTER_MINUTES: Address of the alarm minutes register
 * RV8803_RTC_ALARM_REGISTER_HOURS: Address of the alarm hours register
 * RV8803_RTC_ALARM_REGISTER_WADA: Address of the alarm weekday/day register
 *
 * RV8803_RTC_ALARM_ENABLE_MINUTES: Value to enable the alarm minutes
 * RV8803_RTC_ALARM_ENABLE_HOURS: Value to enable the alarm hours
 * RV8803_RTC_ALARM_ENABLE_WADA: Value to enable the alarm weekday/day
 *
 * RV8803_RTC_ALARM_DISABLE_MINUTES: Value to disable the alarm minutes
 * RV8803_RTC_ALARM_DISABLE_HOURS: Value to disable the alarm hours
 * RV8803_RTC_ALARM_DISABLE_WADA: Value to disable the alarm weekday/day
 *
 * RV8803_RTC_ALARM_MASK_MINUTES: Mask to disable the alarm minutes
 * RV8803_RTC_ALARM_MASK_HOURS: Mask to disable the alarm hours
 * RV8803_RTC_ALARM_MASK_WADA: Mask to disable the alarm weekday/day
 */
#define RV8803_RTC_ALARM_REGISTER_MINUTES 0x08
#define RV8803_RTC_ALARM_REGISTER_HOURS   0x09
#define RV8803_RTC_ALARM_REGISTER_WADA    0x0A
#define RV8803_RTC_ALARM_ENABLE_MINUTES   (0x00 << 7)
#define RV8803_RTC_ALARM_ENABLE_HOURS     (0x00 << 7)
#define RV8803_RTC_ALARM_ENABLE_WADA      (0x00 << 7)
#define RV8803_RTC_ALARM_DISABLE_MINUTES  (0x01 << 7)
#define RV8803_RTC_ALARM_DISABLE_HOURS    (0x01 << 7)
#define RV8803_RTC_ALARM_DISABLE_WADA     (0x01 << 7)
#define RV8803_RTC_ALARM_MASK_MINUTES     RV8803_RTC_ALARM_DISABLE_MINUTES
#define RV8803_RTC_ALARM_MASK_HOURS       RV8803_RTC_ALARM_DISABLE_HOURS
#define RV8803_RTC_ALARM_MASK_WADA        RV8803_RTC_ALARM_DISABLE_WADA

/**
 * @brief Masks for the RTC alarm data
 *
 * These macros are used to access the individual fields of the RTC alarm
 * time structure.
 *
 * RV8803_RTC_SECONDS_BITS: Mask for the seconds field
 * RV8803_RTC_MINUTES_BITS: Mask for the minutes field
 * RV8803_RTC_HOURS_BITS: Mask for the hours field
 * RV8803_RTC_WEEKDAY_BITS: Mask for the weekday/day field
 * RV8803_RTC_DATE_BITS: Mask for the date field
 * RV8803_RTC_MONTH_BITS: Mask for the month field
 * RV8803_RTC_YEAR_BITS: Mask for the year field
 */
#define RV8803_RTC_SECONDS_BITS GENMASK(6, 0)
#define RV8803_RTC_MINUTES_BITS GENMASK(6, 0)
#define RV8803_RTC_HOURS_BITS   GENMASK(5, 0)
#define RV8803_RTC_WEEKDAY_BITS GENMASK(6, 0)
#define RV8803_RTC_DATE_BITS    GENMASK(5, 0)
#define RV8803_RTC_MONTH_BITS   GENMASK(4, 0)
#define RV8803_RTC_YEAR_BITS    GENMASK(7, 0)

/* Control MACRO */
/* Check for partial incrementation when reads get 59 seconds */
#define RV8803_RTC_PARTIAL_SECONDS_INCR 59
#define RV8803_RTC_CORRECT_YEAR_LEAP_MIN                                                           \
	(2000 - 1900) /* Diff between 2000 and tm base year 1900                                   \
		       */
#define RV8803_RTC_CORRECT_YEAR_LEAP_MAX                                                           \
	(2099 - 1900) /* Diff between 2099 and tm base year 1900                                   \
		       */
#define RV8803_RTC_RESET_BIT      (0x01 << 0)
#define RV8803_RTC_ENABLE_ALARM   (0x01 << 3)
#define RV8803_RTC_DISABLE_ALARM  (0x00 << 3)
#define RV8803_RTC_WEEKDAY_ALARM  (0x00 << 6)
#define RV8803_RTC_MONTHDAY_ALARM (0x01 << 6)

#define RV8803_RTC_ENABLE_UPDATE  (0x01 << 5)
#define RV8803_RTC_DISABLE_UPDATE (0x00 << 5)

/**
 * @brief Macros for RTC alarm and update IRQs
 *
 * These macros are used to check if the alarm IRQ is available and enabled
 * in the Devicetree. They are used to conditionally compile the code that
 * handles the respective interrupts.
 *
 * @details
 * RV8803_RTC_IRQ_ALARM_AVAILABLE is 1 if the alarm IRQ is available, 0
 * otherwise.
 *
 * RV8803_RTC_IRQ_ALARM is the enum value of the alarm IRQ as specified in
 * the Devicetree.
 *
 * RV8803_RTC_IRQ_ALARM_HAS_PROP is 1 if the 'use-irq' property is set in the
 * Devicetree, 0 otherwise.
 */
#if defined(CONFIG_RTC_ALARM)
#if RV8803_HAS_IRQ
#define RV8803_RTC_IRQ_ALARM_AVAILABLE      1
#define RV8803_RTC_IRQ_ALARM(inst)          DT_INST_ENUM_IDX(inst, alarm)
#define RV8803_RTC_IRQ_ALARM_HAS_PROP(inst) DT_INST_PROP(inst, use_irq)
#else
#define RV8803_RTC_IRQ_ALARM_AVAILABLE      0
#define RV8803_RTC_IRQ_ALARM(inst)          0
#define RV8803_RTC_IRQ_ALARM_HAS_PROP(inst) 0
#endif /* RV8803_HAS_IRQ */
#else
#define RV8803_RTC_IRQ_ALARM_AVAILABLE      0
#define RV8803_RTC_IRQ_ALARM(inst)          0
#define RV8803_RTC_IRQ_ALARM_HAS_PROP(inst) 0
#endif /* CONFIG_RTC_ALARM */

/**
 * @brief Macros for RTC update IRQs
 *
 * These macros are used to check if the update IRQ is available and enabled
 * in the Devicetree. They are used to conditionally compile the code that
 * handles the respective interrupts.
 *
 * @details
 * RV8803_RTC_IRQ_UPDATE_AVAILABLE is 1 if the update IRQ is available, 0
 * otherwise.
 *
 * RV8803_RTC_IRQ_UPDATE is the enum value of the update IRQ as specified in
 * the Devicetree.
 *
 * RV8803_RTC_IRQ_UPDATE_HAS_PROP is 1 if the 'use-irq' property is set in the
 * Devicetree, 0 otherwise.
 */
#if defined(CONFIG_RTC_UPDATE)
#if RV8803_HAS_IRQ
#define RV8803_RTC_IRQ_UPDATE_AVAILABLE      1
#define RV8803_RTC_IRQ_UPDATE(inst)          DT_INST_ENUM_IDX(inst, update)
#define RV8803_RTC_IRQ_UPDATE_HAS_PROP(inst) DT_INST_PROP(inst, use_irq)
#else
#define RV8803_RTC_IRQ_UPDATE_AVAILABLE      0
#define RV8803_RTC_IRQ_UPDATE(inst)          0
#define RV8803_RTC_IRQ_UPDATE_HAS_PROP(inst) 0
#endif /* RV8803_HAS_IRQ */
#else
#define RV8803_RTC_IRQ_UPDATE_AVAILABLE      0
#define RV8803_RTC_IRQ_UPDATE(inst)          0
#define RV8803_RTC_IRQ_UPDATE_HAS_PROP(inst) 0
#endif /* CONFIG_RTC_UPDATE */

/**
 * @brief Macro to determine if any RTC IRQ is in use
 *
 * This macro checks if either the alarm or update IRQ is available
 * and sets the RV8803_RTC_IRQ_IN_USE flag accordingly. It also defines
 * a macro to check if the 'use-irq' property is set for a given instance.
 */
#if RV8803_RTC_IRQ_ALARM_AVAILABLE || RV8803_RTC_IRQ_UPDATE_AVAILABLE
#define RV8803_RTC_IRQ_IN_USE         1
#define RV8803_RTC_IRQ_HAS_PROP(inst) DT_INST_PROP(inst, use_irq)
#else
#define RV8803_RTC_IRQ_IN_USE         0
#define RV8803_RTC_IRQ_HAS_PROP(inst) 0
#endif

/**
 * @brief Configuration structure for the RV8803 RTC
 *
 * @details This structure contains the following:
 *
 * - @a mfd_dev: Parent device reference
 * - @a irq_alarm: Alarm IRQ in use
 * - @a irq_update: Update IRQ in use
 */
struct rv8803_rtc_config {
	const struct device *mfd_dev;
	const bool irq_alarm;
	const bool irq_update;
};

#if RV8803_RTC_IRQ_IN_USE
/**
 * @brief Structure to hold the Interrupt information
 *
 * @details This structure contains the following:
 *
 * - @a dev: Device reference
 * - @a work: Work callback
 * - @a append_listener_fn: Function to append the listener to the interrupt.
 */
struct rv8803_rtc_irq {
	const struct device *dev;
	struct k_work work;

	/**
	 * @brief Function to append the listener to the interrupt
	 *
	 * @param dev Device reference
	 * @param worker Work callback to append
	 *
	 * @return 0 on success, negative errno code on failure
	 */
	int (*append_listener_fn)(const struct device *dev, struct k_work *worker);
};
#endif /* RV8803_RTC_IRQ_IN_USE */

#if RV8803_RTC_IRQ_ALARM_AVAILABLE
/**
 * @brief Structure to hold the alarm callback information
 *
 * @details This structure contains the following:
 *
 * - @a alarm_cb: Alarm callback function.
 * - @a alarm_cb_data: Alarm callback data pointer.
 */
struct rv8803_rtc_alarm {
	rtc_alarm_callback alarm_cb;
	void *alarm_cb_data;
};
#endif /* RV8803_RTC_IRQ_ALARM_AVAILABLE */

#if RV8803_RTC_IRQ_UPDATE_AVAILABLE
/**
 * @brief Structure to hold the update callback information
 *
 * @details This structure contains the following:
 *
 * - @a update_cb: Update callback function.
 * - @a update_cb_data: Update callback data pointer.
 */
struct rv8803_rtc_update {
	rtc_update_callback update_cb;
	void *update_cb_data;
};
#endif /* RV8803_RTC_IRQ_UPDATE_AVAILABLE */

/**
 * @brief RV8803 RTC data structure
 *
 * @details This structure contains the following:
 *
 * - @a rtc_irq: Pointer to the RV8803 RTC IRQ structure.
 * - @a rtc_alarm: Pointer to the RV8803 RTC alarm structure.
 * - @a rtc_update: Pointer to the RV8803 RTC update structure.
 */
struct rv8803_rtc_data {
	struct rv8803_rtc_irq *rtc_irq;
	struct rv8803_rtc_alarm *rtc_alarm;
	struct rv8803_rtc_update *rtc_update;
};
#endif /* ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_RTC_H_ */
