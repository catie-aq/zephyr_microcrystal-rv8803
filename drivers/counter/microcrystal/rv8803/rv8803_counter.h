/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_COUNTER_H_
#define ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_COUNTER_H_

#include <zephyr/drivers/counter.h>

/* TIMER Counter Registers */
/**
 * @brief Macros for the RV8803 counter
 *
 * These macros are used to access and modify the RV8803 counter registers.
 *
 * - RV8803_COUNTER_REGISTER_TIMER_COUNTER_0: Address of timer counter 0
 * - RV8803_COUNTER_REGISTER_TIMER_COUNTER_1: Address of timer counter 1
 * - RV8803_COUNTER_REGISTER_SHIFT_COUNTER: Shift value to access the counter
 *   enable register field
 * - RV8803_COUNTER_ENABLE_COUNTER: Value to enable the counter
 * - RV8803_COUNTER_DISABLE_COUNTER: Value to disable the counter
 * - RV8803_COUNTER_FREQUENCY_SHIFT_COUNTER: Shift value to access the counter
 *   frequency register field
 * - RV8803_COUNTER_FREQUENCY_MASK_COUNTER: Mask value to access the counter
 *   frequency register field
 */
#define RV8803_COUNTER_REGISTER_TIMER_COUNTER_0 0x0B
#define RV8803_COUNTER_REGISTER_TIMER_COUNTER_1 0x0C

#define RV8803_COUNTER_REGISTER_SHIFT_COUNTER 4
#define RV8803_COUNTER_ENABLE_COUNTER         (0x01 << RV8803_COUNTER_REGISTER_SHIFT_COUNTER)
#define RV8803_COUNTER_DISABLE_COUNTER        (0x00 << RV8803_COUNTER_REGISTER_SHIFT_COUNTER)

#define RV8803_COUNTER_FREQUENCY_SHIFT_COUNTER 0
#define RV8803_COUNTER_FREQUENCY_MASK_COUNTER  (0x03 << RV8803_COUNTER_FREQUENCY_SHIFT_COUNTER)

/**
 * @brief Check if the 'use-irq' property is set for a counter instance
 *
 * This macro evaluates whether the 'use-irq' property is defined
 * for a given counter instance in the Devicetree.
 *
 * @param inst The instance number
 * @return 1 if the property is set, 0 otherwise
 */
#if RV8803_HAS_IRQ
#define RV8803_COUNTER_IRQ_HAS_PROP(inst) DT_INST_PROP(inst, use_irq)
#else
#define RV8803_COUNTER_IRQ_HAS_PROP(inst) 0
#endif /* RV8803_HAS_IRQ */

/**
 * @brief Counter configuration macros
 *
 * These macros define the maximum top value, number of channels, and the
 * frequency values for the RV8803 counter.
 *
 * - RV8803_COUNTER_CHANNELS: Number of channels for the counter
 * - RV8803_COUNTER_MAX_TOP_VALUE: Maximum top value for the counter
 * - RV8803_COUNTER_FREQUENCY_4096_HZ: Counter frequency value for 4096 Hz
 * - RV8803_COUNTER_FREQUENCY_64_HZ: Counter frequency value for 64 Hz
 * - RV8803_COUNTER_FREQUENCY_1_HZ: Counter frequency value for 1 Hz
 * - RV8803_COUNTER_FREQUENCY_1_60_HZ: Counter frequency value for 1/60 Hz
 */
#define RV8803_COUNTER_CHANNELS          1
#define RV8803_COUNTER_MAX_TOP_VALUE     0x0FFFU
#define RV8803_COUNTER_FREQUENCY_4096_HZ 0x00
#define RV8803_COUNTER_FREQUENCY_64_HZ   0x01
#define RV8803_COUNTER_FREQUENCY_1_HZ    0x02
#define RV8803_COUNTER_FREQUENCY_1_60_HZ 0x03

/**
 * @brief RV8803 counter configuration structure
 *
 * @details This structure contains the following:
 *
 * - @a info: Contains the maximum top value, frequency, and number of channels.
 * - @a mfd_dev: Pointer to the parent Multi-Function Device (MFD).
 */
struct rv8803_cnt_config {
	struct counter_config_info info;
	const struct device *mfd_dev;
};

#if RV8803_HAS_IRQ
/**
 * @brief RV8803 counter IRQ structure
 *
 * @details This structure contains the following:
 *
 * - @a dev: Pointer to the device structure.
 * - @a work: Pointer to the k_work structure.
 * - @a append_listener_fn: Function to append the listener to the interrupt.
 */
struct rv8803_cnt_irq {
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

/**
 * @brief RV8803 counter callback structure
 *
 * @details This structure contains the following:
 *
 * - @a counter_cb: Callback function to be called.
 * - @a counter_cb_data: Pointer to user data for the callback function.
 */
struct rv8803_cnt_counter {
	counter_top_callback_t counter_cb;
	void *counter_cb_data;
};
#endif

/**
 * @brief RV8803 counter data structure
 *
 * @details This structure contains the following:
 *
 * - @a cnt_irq: Pointer to the RV8803 counter IRQ structure.
 * - @a cnt_counter: Pointer to the RV8803 counter structure.
 */
struct rv8803_cnt_data {
	struct rv8803_cnt_irq *cnt_irq;
	struct rv8803_cnt_counter *cnt_counter;
};
#endif /* ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_COUNTER_H_ */
