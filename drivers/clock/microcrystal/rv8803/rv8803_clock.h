/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_CLOCK_H_
#define ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_CLOCK_H_

#include <zephyr/drivers/clock_control.h>

/**
 * @brief Macros to control the clock frequency.
 *
 * These macros are used to set the clock frequency of the RV8803 device.
 * The clock frequency is set by writing the frequency value to the
 * @ref RV8803_CLOCK_FREQUENCY register.
 *
 * @details The clock frequency is set by writing the frequency value to the
 * bits 2-3 of the @ref RV8803_CLOCK_FREQUENCY register. The possible values
 * are:
 *
 * - @ref RV8803_CLOCK_FREQUENCY_32768_HZ: 32768 Hz
 * - @ref RV8803_CLOCK_FREQUENCY_1024_HZ: 1024 Hz
 * - @ref RV8803_CLOCK_FREQUENCY_1_HZ: 1 Hz
 *
 * The clock frequency can be set using the @ref rv8803_clk_set_rate()
 * function.
 */
#define RV8803_CLOCK_FREQUENCY_SHIFT    2
#define RV8803_CLOCK_FREQUENCY_MASK     (0x03 << RV8803_CLOCK_FREQUENCY_SHIFT)
#define RV8803_CLOCK_FREQUENCY_32768_HZ 0x00
#define RV8803_CLOCK_FREQUENCY_1024_HZ  0x01
#define RV8803_CLOCK_FREQUENCY_1_HZ     0x02

/**
 * @brief RV8803 clock configuration structure.
 *
 * @details This structure contains the following:
 *
 * - @a mfd_dev Parent device reference.
 */
struct rv8803_clk_config {
	const struct device *mfd_dev;
};

/** @brief RV8803 clock data structure. */
struct rv8803_clk_data {
};
#endif /* ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_CLOCK_H_ */
