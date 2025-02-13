/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_CLOCK_H_
#define ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_CLOCK_H_

#include <zephyr/drivers/clock_control.h>

#define RV8803_CLOCK_FREQUENCY_SHIFT    2
#define RV8803_CLOCK_FREQUENCY_MASK     (0x03 << RV8803_CLOCK_FREQUENCY_SHIFT)
#define RV8803_CLOCK_FREQUENCY_32768_HZ 0x00
#define RV8803_CLOCK_FREQUENCY_1024_HZ  0x01
#define RV8803_CLOCK_FREQUENCY_1_HZ     0x02

/* Structs */
/* RV8803 CLK config */
struct rv8803_clk_config {
	const struct device *mfd_dev;
};

/* RV8803 CLK data */
struct rv8803_clk_data {
};
#endif /* ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_CLOCK_H_ */
