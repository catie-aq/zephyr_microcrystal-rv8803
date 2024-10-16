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

#define RV8803_COUNTER_CHANNELS          1
#define RV8803_COUNTER_MAX_TOP_VALUE     0x0FFFU
#define RV8803_COUNTER_FREQUENCY_4096_HZ 0x00
#define RV8803_COUNTER_FREQUENCY_64_HZ   0x01
#define RV8803_COUNTER_FREQUENCY_1_HZ    0x02
#define RV8803_COUNTER_FREQUENCY_1_60_HZ 0x03

static const uint16_t rv8803_frequency[3] = {4096, 64, 1}; /* Not supported by Zephyr < 1Hz */

/* RV8803 CLK config */
struct rv8803_cnt_config {
	struct counter_config_info info; /* WARNING counter_config_info have to be first for config
					    casting in counter api */
	const struct device *base_dev;   /* Parent device reference */
};

/* RV8803 CLK data */
struct rv8803_cnt_data {
};
#endif
#endif /* ZEPHYR_DRIVERS_RTC_RV8803_CNT_H_ */
