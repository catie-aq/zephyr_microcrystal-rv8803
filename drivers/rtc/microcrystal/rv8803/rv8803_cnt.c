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
static int counter_rv8803_cnt_start(const struct device *dev)
{
	return 0;
}

static int counter_rv8803_cnt_stop(const struct device *dev)
{
	return 0;
}

static int counter_rv8803_cnt_get_value(const struct device *dev, uint32_t *ticks)
{
	return 0;
}

static int counter_rv8803_cnt_set_alarm(const struct device *dev, uint8_t chan_id,
					const struct counter_alarm_cfg *alarm_cfg)
{
	return 0;
}

static int counter_rv8803_cnt_cancel_alarm(const struct device *dev, uint8_t chan_id)
{
	return 0;
}

static int counter_rv8803_cnt_set_top_value(const struct device *dev,
					    const struct counter_top_cfg *cfg)
{
	return 0;
}

static uint32_t counter_rv8803_cnt_get_top_value(const struct device *dev)
{
	return 0;
}

static uint32_t counter_rv8803_cnt_get_pending_int(const struct device *dev)
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
	.start = counter_rv8803_cnt_start,
	.stop = counter_rv8803_cnt_stop,
	.get_value = counter_rv8803_cnt_get_value,
	.set_alarm = counter_rv8803_cnt_set_alarm,
	.cancel_alarm = counter_rv8803_cnt_cancel_alarm,
	.set_top_value = counter_rv8803_cnt_set_top_value,
	.get_top_value = counter_rv8803_cnt_get_top_value,
	.get_pending_int = counter_rv8803_cnt_get_pending_int,
};

/* RV8803 CNT Initialization MACRO */
#define RV8803_CNT_INIT(n)                                                                         \
	static const struct rv8803_cnt_config rv8803_cnt_config_##n = {                            \
		.base_dev = DEVICE_DT_GET(DT_PARENT(DT_INST(n, DT_DRV_COMPAT))),                   \
	};                                                                                         \
	static struct rv8803_cnt_data rv8803_cnt_data_##n;                                         \
	DEVICE_DT_INST_DEFINE(n, rv8803_cnt_init, NULL, &rv8803_cnt_data_##n,                      \
			      &rv8803_cnt_config_##n, POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY,   \
			      &rv8803_cnt_driver_api);

/* Instanciate RV8803 CNT */
DT_INST_FOREACH_STATUS_OKAY(RV8803_CNT_INIT)
#endif /* CONFIG_COUNTER && CONFIG_RV8803_CNT_ENABLE */
#undef DT_DRV_COMPAT
