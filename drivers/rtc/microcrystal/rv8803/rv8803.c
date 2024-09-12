/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microcrystal_rv8803

#include <zephyr/logging/log.h>

#include "rv8803.h"

LOG_MODULE_REGISTER(RV8803, CONFIG_RTC_LOG_LEVEL);

/* RV8803 base init */
static int rv8803_init(const struct device *dev)
{
	const struct rv8803_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->i2c_bus)) {
		LOG_ERR("I2C bus not ready!!");
		return -ENODEV;
	}
	LOG_INF("RV8803 INIT");

	return 0;
}

/* Device Initialization MACRO */
#define RV8803_INIT(n)                                                                             \
	static const struct rv8803_config rv8803_config_##n = {                                    \
		.i2c_bus = I2C_DT_SPEC_INST_GET(n),                                                \
	};                                                                                         \
	static struct rv8803_data rv8803_data_##n;                                                 \
	DEVICE_DT_INST_DEFINE(n, rv8803_init, NULL, &rv8803_data_##n, &rv8803_config_##n,          \
			      POST_KERNEL, CONFIG_RTC_INIT_PRIORITY, NULL);

/* Instanciate RV8803 */
DT_INST_FOREACH_STATUS_OKAY(RV8803_INIT)
#undef DT_DRV_COMPAT
