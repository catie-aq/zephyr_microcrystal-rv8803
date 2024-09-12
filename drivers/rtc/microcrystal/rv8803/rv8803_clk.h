/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_RTC_RV8803_CLK_H_
#define ZEPHYR_DRIVERS_RTC_RV8803_CLK_H_

/* CLK OUT control*/
#define RV8803_CLK_FREQUENCY_SHIFT    2
#define RV8803_CLK_FREQUENCY_MASK     (0x03 << RV8803_CLK_FREQUENCY_SHIFT)
#define RV8803_CLK_FREQUENCY_32768_HZ 0x00
#define RV8803_CLK_FREQUENCY_1024_HZ  0x01
#define RV8803_CLK_FREQUENCY_1_HZ     0x02

#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(clk_gpios)
#define RV8803_CLK_GPIO_IN_USE 1
#endif

/* Structs */
#if CONFIG_CLOCK_CONTROL && CONFIG_RV8803_CLK_ENABLE
/* RV8803 CLK config */
struct rv8803_clk_config {
	const struct device *base_dev; // Parent device reference
};

/* RV8803 CLK data */
struct rv8803_clk_data {
};
#endif
#endif /* ZEPHYR_DRIVERS_RTC_RV8803_CLK_H_ */
