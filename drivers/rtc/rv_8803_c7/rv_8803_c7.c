/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microcrystal_rv_8803_c7

#include <zephyr/drivers/sensor.h>
#include <zephyr/logging/log.h>

#include "rv_8803_c7.h"

LOG_MODULE_REGISTER(RV_8803_C7, CONFIG_SENSOR_LOG_LEVEL);

struct rv_8803_c7_config {
};

struct rv_8803_c7_data {
};

static int rv_8803_c7_attr_set(const struct device *dev, enum sensor_channel chan,
			   enum sensor_attribute attr, const struct sensor_value *val)
{
	return 0;
}

static int rv_8803_c7_sample_fetch(const struct device *dev, enum sensor_channel chan)
{
	struct rv_8803_c7_data *data = dev->data;
	const struct rv_8803_c7_config *config = dev->config;

	return 0;
}

static int rv_8803_c7_channel_get(const struct device *dev, enum sensor_channel chan,
			      struct sensor_value *val)
{
	struct rv_8803_c7_data *data = dev->data;

    // TODO: Update val with the sensor value
	val->val1 = 0;
	val->val2 = 0;

	return 0;
}

static int rv_8803_c7_init(const struct device *dev)
{
	const struct rv_8803_c7_config *config = dev->config;
	struct rv_8803_c7_data *data = dev->data;

	return 0;
}

static const struct sensor_driver_api rv_8803_c7_driver_api = {
	.attr_set = rv_8803_c7_attr_set,
	.sample_fetch = rv_8803_c7_sample_fetch,
	.channel_get = rv_8803_c7_channel_get,
};

#define RV_8803_C7_INIT(n)                                                                             \
	static struct rv_8803_c7_config rv_8803_c7_config_##n = {                                             \
	};                                                                                         \
	static struct rv_8803_c7_data rv_8803_c7_data_##n;                                                 \
	DEVICE_DT_INST_DEFINE(n, rv_8803_c7_init, NULL, &rv_8803_c7_data_##n, &rv_8803_c7_config_##n,          \
			      POST_KERNEL, CONFIG_SENSOR_INIT_PRIORITY, &rv_8803_c7_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RV_8803_C7_INIT)
