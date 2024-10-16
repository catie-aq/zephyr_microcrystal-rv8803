/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microcrystal_rv8803_cnt

#include <zephyr/drivers/counter.h>
#include <zephyr/logging/log.h>

#include "rv8803.h"
#include "rv8803_cnt.h"

LOG_MODULE_REGISTER(RV8803_CNT, CONFIG_RTC_LOG_LEVEL);

#if CONFIG_COUNTER && CONFIG_RV8803_COUNTER_ENABLE
static int rv8803_cnt_start(const struct device *dev)
{
	return 0;
}

static int rv8803_cnt_stop(const struct device *dev)
{
	return 0;
}

static int rv8803_cnt_set_top_value(const struct device *dev, const struct counter_top_cfg *cfg)
{
	return 0;
}

static uint32_t rv8803_cnt_get_top_value(const struct device *dev)
{
	return 0;
}

static uint32_t rv8803_cnt_get_max_top_value(const struct device *dev)
{
	return 0;
}

static uint32_t rv8803_cnt_get_pending_int(const struct device *dev)
{
	return 0;
}

static uint32_t rv8803_cnt_us_to_ticks(const struct device *dev, uint64_t us)
{
	return 0;
}

static uint64_t rv8803_cnt_ticks_to_us(const struct device *dev, uint32_t ticks)
{
	return 0;
}

/* RV8803 CNT init */
static int rv8803_cnt_init(const struct device *dev)
{
	const struct rv8803_cnt_config *config = dev->config;

	if (!device_is_ready(config->base_dev)) {
		return -ENODEV;
	}
	LOG_INF("RV8803 CNT INIT");

	return 0;
}

/* RV8803 CNT driver API */
static const struct counter_driver_api rv8803_cnt_driver_api = {
	.start = rv8803_cnt_start,
	.stop = rv8803_cnt_stop,
	.set_top_value = rv8803_cnt_set_top_value,
	.get_top_value = rv8803_cnt_get_top_value,
	.get_max_top_value = rv8803_cnt_get_max_top_value,
	.get_pending_int = rv8803_cnt_get_pending_int,
	.us_to_ticks = rv8803_cnt_us_to_ticks,
	.ticks_to_us = rv8803_cnt_ticks_to_us,
};

/* RV8803 CNT Initialization MACRO */
#define RV8803_CNT_INIT(n)                                                                         \
	static const struct rv8803_cnt_config rv8803_cnt_config_##n = {                            \
		.base_dev = DEVICE_DT_GET(DT_PARENT(DT_INST(n, DT_DRV_COMPAT))),                   \
		.freq = DT_INST_ENUM_IDX(n, frequency),                                            \
	};                                                                                         \
	static struct rv8803_cnt_data rv8803_cnt_data_##n;                                         \
	DEVICE_DT_INST_DEFINE(n, rv8803_cnt_init, NULL, &rv8803_cnt_data_##n,                      \
			      &rv8803_cnt_config_##n, POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY,   \
			      &rv8803_cnt_driver_api);

/* Instanciate RV8803 CNT */
DT_INST_FOREACH_STATUS_OKAY(RV8803_CNT_INIT)
#endif /* CONFIG_COUNTER && CONFIG_RV8803_CNT_ENABLE */
#undef DT_DRV_COMPAT
