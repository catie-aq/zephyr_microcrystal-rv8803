/*
 * Copyright (c) 2025, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_MFD_H_
#define ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_MFD_H_

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>

/**
 * @brief Timing constraint for the startup of the RV8803 device.
 *
 * This macro defines the startup timing constraint for the RV8803 device in
 * milliseconds. The startup timing constraint is the time it takes for the
 * device to start up after the VCC voltage is applied.
 *
 * @details The startup timing constraint is used to delay the start of the
 * device until the VCC voltage is stable.
 */
#define RV8803_STARTUP_TIMING_MS 80

/**
 * @brief Flag mask for low voltage.
 *
 * This macro defines the bit masks for the low voltage flags of the RV8803
 * device. The two flags are:
 *
 * - RV8803_FLAG_MASK_LOW_VOLTAGE_1: Bit 0 of the flag register.
 * - RV8803_FLAG_MASK_LOW_VOLTAGE_2: Bit 1 of the flag register.
 *
 * The low voltage flags are used to detect when the battery voltage is too
 * low and the device should be shutdown.
 */
#define RV8803_FLAG_MASK_LOW_VOLTAGE_1 (0x01 << 0)
#define RV8803_FLAG_MASK_LOW_VOLTAGE_2 (0x01 << 1)

/**
 * @brief Generate a macro to count the number of children of a given instance
 * that have a given property and use IRQ.
 *
 * @details This macro takes an instance number and a property name and
 * generates a macro that takes an instance number and evaluates to the
 * number of children of the given instance that have the given property and
 * use IRQ.
 *
 * @param inst The instance number.
 *
 * @return The number of children of the given instance that have the given
 * property and use IRQ.
 */
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

/**
 * @brief RV8803 configuration structure.
 *
 * @details This structure contains the following:
 *
 * - @a i2c_bus: I2C bus configuration.
 * - @a irq_gpio: GPIO configuration for the interrupt line.
 */
struct rv8803_config {
	struct i2c_dt_spec i2c_bus;
#if RV8803_HAS_IRQ
	const struct gpio_dt_spec irq_gpio;
#endif
};

#if RV8803_HAS_IRQ
/**
 * @brief RV8803 IRQ structure.
 *
 * @details This structure contains the following:
 *
 * - @a gpio_cb: GPIO callback structure.
 * - @a workers: Array of work pointers.
 * - @a workers_index: Index of the current work pointer.
 * - @a max_workers: Maximum number of work pointers.
 */
struct rv8803_irq {
	struct gpio_callback gpio_cb;
	struct k_work **workers;
	int workers_index;
	const int max_workers;
};
#endif /* RV8803_HAS_IRQ */

/**
 * @brief RV8803 data structure.
 *
 * @details This structure contains the following:
 *
 * - @a bat: RV8803 battery information structure.
 * - @a irq: Pointer to the RV8803 IRQ structure.
 */
struct rv8803_data {
#if CONFIG_MFD_RV8803_DETECT_BATTERY_STATE
	struct rv8803_battery bat;
#endif
	struct rv8803_irq *irq;
};

#endif /* ZEPHYR_DRIVERS_RTC_MICROCYSTAL_RV8803_MFD_H_ */
