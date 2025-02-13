/*
 * Copyright (c) 2025, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_MFD_H_
#define ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_MFD_H_

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>

#define RV8803_STARTUP_TIMING_MS 80

#define RV8803_FLAG_MASK_LOW_VOLTAGE_1 (0x01 << 0)
#define RV8803_FLAG_MASK_LOW_VOLTAGE_2 (0x01 << 1)

#define RV8803_DT_NUM_CHILD_IRQ(inst) RV8803_DT_CHILD_CNT_BOOL_STATUS_OKAY(inst, use_irq) 0

#if RV8803_HAS_IRQ
#define RV8803_DT_CHILD_HAS_IRQ(inst)                                                              \
	COND_CODE_1(IS_EQ(RV8803_DT_CHILD_CNT_BOOL_STATUS_OKAY(inst, use_irq) 0, 0), (0), (1))
#else
#define RV8803_DT_CHILD_HAS_IRQ(inst) 0
#if RV8803_DT_ANY_INST_CHILD_CNT_BOOL_STATUS_OKAY(use_irq)
#error RV8803 IRQ missing definition
#endif
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(irq_gpios)
#warning RV8803 IRQ defined but not used
#endif
#endif

struct rv8803_config {
	struct i2c_dt_spec i2c_bus;
#if RV8803_HAS_IRQ
	const struct gpio_dt_spec irq_gpio;
#endif
};

#if RV8803_HAS_IRQ
struct rv8803_irq {
	struct gpio_callback gpio_cb;
	struct k_work **workers;
	int workers_index;
	const int max_workers;
};
#endif /* RV8803_HAS_IRQ */

struct rv8803_data {
#if CONFIG_MFD_RV8803_DETECT_BATTERY_STATE
	struct rv8803_battery bat;
#endif
	struct rv8803_irq *irq;
};

#endif /* ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_MFD_H_ */
