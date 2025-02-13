/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_COUNTER_H_
#define ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_COUNTER_H_

/* TIMER Counter Registers */
#define RV8803_REGISTER_TIMER_COUNTER_0 0x0B
#define RV8803_REGISTER_TIMER_COUNTER_1 0x0C

#define RV8803_REGISTER_SHIFT_COUNTER 4
#define RV8803_EXTENSION_MASK_COUNTER (0x01 << RV8803_REGISTER_SHIFT_COUNTER)
#define RV8803_FLAG_MASK_COUNTER      (0x01 << RV8803_REGISTER_SHIFT_COUNTER)
#define RV8803_CONTROL_MASK_COUNTER   (0x01 << RV8803_REGISTER_SHIFT_COUNTER)

#define RV8803_ENABLE_COUNTER  (0x01 << RV8803_REGISTER_SHIFT_COUNTER)
#define RV8803_DISABLE_COUNTER (0x00 << RV8803_REGISTER_SHIFT_COUNTER)

#define RV8803_FREQUENCY_SHIFT_COUNTER 0
#define RV8803_FREQUENCY_MASK_COUNTER  (0x03 << RV8803_FREQUENCY_SHIFT_COUNTER)

#if RV8803_HAS_IRQ
#define RV8803_COUNTER_IRQ_HAS_PROP(inst) DT_INST_PROP(inst, use_irq)
#else
#define RV8803_COUNTER_IRQ_HAS_PROP(inst) 0
#endif /* RV8803_HAS_IRQ */

#define RV8803_COUNTER_CHANNELS          1
#define RV8803_COUNTER_MAX_TOP_VALUE     0x0FFFU
#define RV8803_COUNTER_FREQUENCY_4096_HZ 0x00
#define RV8803_COUNTER_FREQUENCY_64_HZ   0x01
#define RV8803_COUNTER_FREQUENCY_1_HZ    0x02
#define RV8803_COUNTER_FREQUENCY_1_60_HZ 0x03

static const uint16_t rv8803_frequency[3] = {4096, 64, 1}; /* Not supported by Zephyr < 1Hz */

/* RV8803 CLK config */
struct rv8803_cnt_config {
	struct counter_config_info info;
	const struct device *mfd_dev;
};

#if RV8803_HAS_IRQ
// clang-format off
struct rv8803_cnt_irq {
	const struct device *dev;
	struct k_work work;
	int (*append_listener_fn)(const struct device *dev, struct k_work *worker);
};
// clang-format on

struct rv8803_cnt_counter {
	counter_top_callback_t counter_cb;
	void *counter_cb_data;
};
#endif

struct rv8803_cnt_data {
	struct rv8803_cnt_irq *cnt_irq;
	struct rv8803_cnt_counter *cnt_counter;
};
#endif /* ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_COUNTER_H_ */
