/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include "rv8803_api.h"
#define DT_DRV_COMPAT microcrystal_rv8803_clock
#include "rv8803_clock.h"

LOG_MODULE_REGISTER(RV8803_CLK, CONFIG_RTC_LOG_LEVEL);

/**
 * @brief Set the clock rate.
 *
 * This function sets the clock rate of the RV8803 clock output.
 *
 * @param dev Pointer to the device structure.
 * @param sys The clock subsystem to set the rate for.
 * @param rate The clock rate to set.
 *
 * @retval 0 If successful.
 * @retval -EALREADY If the clock rate is already set.
 * @retval -ENOTSUP If the clock rate is not supported.
 */
static int rv8803_clk_set_rate(const struct device *dev, clock_control_subsys_t sys,
			       clock_control_subsys_rate_t rate)
{
	ARG_UNUSED(sys);
	const struct rv8803_clk_config *clk_config = dev->config;
	uint8_t reg;
	int err;

	err = rv8803_bus_reg_read_byte(clk_config->mfd_dev, RV8803_REGISTER_EXTENSION, &reg);
	if (err < 0) {
		return err;
	}

	uintptr_t u_rate = (uintptr_t)rate;
	if ((reg & RV8803_CLOCK_FREQUENCY_MASK) == (u_rate << RV8803_CLOCK_FREQUENCY_SHIFT)) {
		return -EALREADY;
	}

	reg &= ~RV8803_CLOCK_FREQUENCY_MASK;
	switch (u_rate) {
	case RV8803_CLOCK_FREQUENCY_32768_HZ:
		reg |= (RV8803_CLOCK_FREQUENCY_32768_HZ << RV8803_CLOCK_FREQUENCY_SHIFT) &
		       RV8803_CLOCK_FREQUENCY_MASK;
		break;

	case RV8803_CLOCK_FREQUENCY_1024_HZ:
		reg |= (RV8803_CLOCK_FREQUENCY_1024_HZ << RV8803_CLOCK_FREQUENCY_SHIFT) &
		       RV8803_CLOCK_FREQUENCY_MASK;
		break;

	case RV8803_CLOCK_FREQUENCY_1_HZ:
		reg |= (RV8803_CLOCK_FREQUENCY_1_HZ << RV8803_CLOCK_FREQUENCY_SHIFT) &
		       RV8803_CLOCK_FREQUENCY_MASK;
		break;

	default:
		return -ENOTSUP;
	}

	err = rv8803_bus_reg_write_byte(clk_config->mfd_dev, RV8803_REGISTER_EXTENSION, reg);
	if (err < 0) {
		return err;
	}

	return 0;
}

/**
 * @brief Get the current clock rate.
 *
 * This function returns the current clock rate of the RV8803 clock output.
 *
 * @param dev Pointer to the device structure.
 * @param sys The clock subsystem to get the rate for.
 * @param rate Pointer to a uint32_t which will store the clock rate.
 *
 * @retval 0 If successful.
 */
static int rv8803_clk_get_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t *rate)
{
	ARG_UNUSED(sys);
	const struct rv8803_clk_config *clk_config = dev->config;
	uint8_t reg;
	int err;

	err = rv8803_bus_reg_read_byte(clk_config->mfd_dev, RV8803_REGISTER_EXTENSION, &reg);
	if (err < 0) {
		return err;
	}

	reg = reg & RV8803_CLOCK_FREQUENCY_MASK;
	*rate = reg >> RV8803_CLOCK_FREQUENCY_SHIFT;

	return 0;
}

/**
 * @brief Initialize the RV8803 clock device.
 *
 * This function initializes the RV8803 clock device by checking if the
 * MFD device is ready.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 on success
 * @retval -ENODEV if the MFD device is not ready
 */
static int rv8803_clk_init(const struct device *dev)
{
	const struct rv8803_clk_config *config = dev->config;

	if (!device_is_ready(config->mfd_dev)) {
		return -ENODEV;
	}
	LOG_INF("RV8803 CLK INIT");

	return 0;
}

/**
 * @brief Clock control driver API structure for RV8803.
 *
 * @details This structure contains the following:
 *
 * - @a set_rate: Function to set the clock rate.
 * - @a get_rate: Function to get the current clock rate.
 */
static const struct clock_control_driver_api rv8803_clk_driver_api = {
	.set_rate = rv8803_clk_set_rate,
	.get_rate = rv8803_clk_get_rate,
};

/**
 * @brief Macro to initialize the RV8803 clock configuration struct.
 *
 * This macro initializes the rv8803_clk_config struct with the parent
 * device reference obtained from the Devicetree.
 *
 * @param n The instance number.
 */
#define RV8803_CLOCK_CONFIG_INIT(n)                                                                \
	static const struct rv8803_clk_config rv8803_clk_config_##n = {                            \
		.mfd_dev = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(n))),                               \
	};

/**
 * @brief Generate the rv8803_clk_data struct for the given instance.
 *
 * This macro generates the rv8803_clk_data struct for the given instance.
 *
 * @param n The instance number.
 */
#define RV8803_CLOCK_DATA_INIT(n) static struct rv8803_clk_data rv8803_clk_data_##n;

/**
 * @brief Macro to initialize the RV8803 clock instance.
 *
 * This macro initializes the RV8803 clock configuration and data structures,
 * and registers the device with the kernel for the given instance.
 *
 * @param n The instance number
 */
#define RV8803_CLK_INIT(n)                                                                         \
	RV8803_CLOCK_CONFIG_INIT(n)                                                                \
	RV8803_CLOCK_DATA_INIT(n)                                                                  \
	DEVICE_DT_INST_DEFINE(n, rv8803_clk_init, NULL, &rv8803_clk_data_##n,                      \
			      &rv8803_clk_config_##n, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY,       \
			      &rv8803_clk_driver_api);

/**
 * @brief Generate the RV8803 clock instances.
 *
 * This macro is used to generate the RV8803 clock instances. It is used to
 * iterate over all instances of the RV8803 clock in the Devicetree. For each
 * instance, the RV8803_CLK_INIT macro is called to initialize the structs and
 * register the device with the kernel.
 *
 * @details The macro DT_INST_FOREACH_STATUS_OKAY is used to iterate over all
 * instances of the RV8803 clock in the Devicetree. For each instance,
 * the RV8803_CLK_INIT macro is called to initialize the structs and
 * register the device.
 */
DT_INST_FOREACH_STATUS_OKAY(RV8803_CLK_INIT)
#undef DT_DRV_COMPAT
