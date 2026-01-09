/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microcrystal_rv8803_clk_catie

#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>

#include "rv8803.h"
#include "rv8803_clk.h"

LOG_MODULE_REGISTER(RV8803_CLK, CONFIG_RTC_LOG_LEVEL);

#if CONFIG_CLOCK_CONTROL && CONFIG_RV8803_CLK_ENABLE
static int rv8803_clk_on(const struct device *dev, clock_control_subsys_t sys)
{
	return -ENOTSUP;
}

static int rv8803_clk_off(const struct device *dev, clock_control_subsys_t sys)
{
	return -ENOTSUP;
}

static int rv8803_clk_set_rate(const struct device *dev, clock_control_subsys_t sys,
			       clock_control_subsys_rate_t rate)
{
	ARG_UNUSED(sys);
	const struct rv8803_clk_config *clk_config = dev->config;
	const struct rv8803_config *config = clk_config->base_dev->config;
	uint8_t reg;
	int err;

	err = i2c_reg_read_byte_dt(&config->i2c_bus, RV8803_REGISTER_EXTENSION, &reg);
	if (err < 0) {
		return err;
	}

	uintptr_t u_rate = (uintptr_t)rate;
	if ((reg & RV8803_CLK_FREQUENCY_MASK) == (u_rate << RV8803_CLK_FREQUENCY_SHIFT)) {
		return -EALREADY;
	}

	reg &= ~RV8803_CLK_FREQUENCY_MASK;
	switch (u_rate) {
	case RV8803_CLK_FREQUENCY_32768_HZ:
		reg |= (RV8803_CLK_FREQUENCY_32768_HZ << RV8803_CLK_FREQUENCY_SHIFT) &
		       RV8803_CLK_FREQUENCY_MASK;
		break;

	case RV8803_CLK_FREQUENCY_1024_HZ:
		reg |= (RV8803_CLK_FREQUENCY_1024_HZ << RV8803_CLK_FREQUENCY_SHIFT) &
		       RV8803_CLK_FREQUENCY_MASK;
		break;

	case RV8803_CLK_FREQUENCY_1_HZ:
		reg |= (RV8803_CLK_FREQUENCY_1_HZ << RV8803_CLK_FREQUENCY_SHIFT) &
		       RV8803_CLK_FREQUENCY_MASK;
		break;

	default:
		return -ENOTSUP;
	}

	err = i2c_reg_write_byte_dt(&config->i2c_bus, RV8803_REGISTER_EXTENSION, reg);
	if (err < 0) {
		return err;
	}

	return 0;
}

static int rv8803_clk_get_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t *rate)
{
	ARG_UNUSED(sys);
	const struct rv8803_clk_config *clk_config = dev->config;
	const struct rv8803_config *config = clk_config->base_dev->config;
	uint8_t reg;
	int err;

	err = i2c_reg_read_byte_dt(&config->i2c_bus, RV8803_REGISTER_EXTENSION, &reg);
	if (err < 0) {
		return err;
	}

	reg = reg & RV8803_CLK_FREQUENCY_MASK;
	*rate = reg >> RV8803_CLK_FREQUENCY_SHIFT;

	return 0;
}

#endif // CONFIG_CLOCK_CONTROL

#if CONFIG_CLOCK_CONTROL && CONFIG_RV8803_CLK_ENABLE
/* RV8803 CLK init */
static int rv8803_clk_init(const struct device *dev)
{
	const struct rv8803_clk_config *config = dev->config;

	if (!device_is_ready(config->base_dev)) {
		return -ENODEV;
	}
	LOG_INF("RV8803 CLK INIT");

	return 0;
}

/* RV8803 RTC driver API */
static const struct clock_control_driver_api rv8803_clk_driver_api = {
	.on = rv8803_clk_on,
	.off = rv8803_clk_off,
	.set_rate = rv8803_clk_set_rate,
	.get_rate = rv8803_clk_get_rate,
};
#endif

#if CONFIG_CLOCK_CONTROL && CONFIG_RV8803_CLK_ENABLE
/* RV8803 CLK Initialization MACRO */
#define RV8803_CLK_INIT(n)                                                                         \
	static const struct rv8803_clk_config rv8803_clk_config_##n = {                            \
		.base_dev = DEVICE_DT_GET(DT_PARENT(DT_INST(n, DT_DRV_COMPAT))),                   \
	};                                                                                         \
	static struct rv8803_clk_data rv8803_clk_data_##n;                                         \
	DEVICE_DT_INST_DEFINE(n, rv8803_clk_init, NULL, &rv8803_clk_data_##n,                      \
			      &rv8803_clk_config_##n, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY,       \
			      &rv8803_clk_driver_api);
#endif

#if CONFIG_CLOCK_CONTROL && CONFIG_RV8803_CLK_ENABLE
/* Instanciate RV8803 CLK */
DT_INST_FOREACH_STATUS_OKAY(RV8803_CLK_INIT)
#endif
#undef DT_DRV_COMPAT
