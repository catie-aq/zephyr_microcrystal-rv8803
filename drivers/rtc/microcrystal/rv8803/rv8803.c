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
	int err;

	if (!i2c_is_ready_dt(&config->i2c_bus)) {
		LOG_ERR("I2C bus not ready!!");
		return -ENODEV;
	}

	k_sleep(K_MSEC(RV8803_STARTUP_TIMING_MS));

#if CONFIG_RV8803_DETECT_BATTERY_STATE
	struct rv8803_data *data = dev->data;
	uint8_t value;
	int err = i2c_reg_read_byte_dt(&config->i2c_bus, RV8803_REGISTER_FLAG, &value);
	if (err < 0) {
		LOG_ERR("Failed to read FLAGS register!!");
		return err;
	}
	LOG_DBG("FLAG REGISTER: [0x%02X]",
		value & (RV8803_FLAG_MASK_LOW_VOLTAGE_1 | RV8803_FLAG_MASK_LOW_VOLTAGE_2));
	data->power_on_reset = value & RV8803_FLAG_MASK_LOW_VOLTAGE_2;
	data->low_battery = value & RV8803_FLAG_MASK_LOW_VOLTAGE_1;

	if (data->power_on_reset) {
		value &= ~RV8803_FLAG_MASK_LOW_VOLTAGE_2;
		err = i2c_reg_write_byte_dt(&config->i2c_bus, RV8803_REGISTER_FLAG, value);
		if (err < 0) {
			LOG_ERR("Failed to write FLAGS register!!");
			return err;
		}
	} else if (data->low_battery) {
		value &= ~RV8803_FLAG_MASK_LOW_VOLTAGE_1;
		err = i2c_reg_write_byte_dt(&config->i2c_bus, RV8803_REGISTER_FLAG, value);
		if (err < 0) {
			LOG_ERR("Failed to write FLAGS register!!");
			return err;
		}
	}
#endif /* CONFIG_RV8803_DETECT_BATTERY_STATE */

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
