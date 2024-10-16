/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_RTC_RV8803_CNT_H_
#define ZEPHYR_DRIVERS_RTC_RV8803_CNT_H_

/* TIMER Counter Registers */
#define RV8803_REGISTER_TIMER_COUNTER_0 0x0B
#define RV8803_REGISTER_TIMER_COUNTER_1 0x0C

#define RV8803_EXTENSION_MASK_UPDATE (0x01 << 5)
#define RV8803_FLAG_MASK_UPDATE      (0x01 << 5)
#define RV8803_CONTROL_MASK_UPDATE   (0x01 << 5)

/* Structs */
#if CONFIG_COUNTER && CONFIG_RV8803_COUNTER_ENABLE

enum frequency_selection {
	RV8803_4096_HZ = RV8803_COUNTER_FREQUENCY_4096_HZ,
	RV8803_64_HZ = RV8803_COUNTER_FREQUENCY_64_HZ,
	RV8803_1_HZ = RV8803_COUNTER_FREQUENCY_1_HZ,
	RV8803_1_60_HZ = RV8803_COUNTER_FREQUENCY_1_60_HZ,
};

/* RV8803 CLK config */
struct rv8803_cnt_config {
	const struct device *base_dev; // Parent device reference
	enum frequency_selection freq;
};

/* RV8803 CLK data */
struct rv8803_cnt_data {
};
#endif
#endif /* ZEPHYR_DRIVERS_RTC_RV8803_CNT_H_ */
