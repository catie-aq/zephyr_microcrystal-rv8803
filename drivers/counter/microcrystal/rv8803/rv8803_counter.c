/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/drivers/counter.h>
#include <zephyr/logging/log.h>

#include "rv8803_api.h"
#define DT_DRV_COMPAT microcrystal_rv8803_counter
#include "rv8803_counter.h"

LOG_MODULE_REGISTER(RV8803_CNT, CONFIG_RTC_LOG_LEVEL);

static int rv8803_cnt_start(const struct device *dev)
{
	const struct rv8803_cnt_config *cnt_config = dev->config;
	int err;

	err = rv8803_bus_reg_update_byte(cnt_config->mfd_dev, RV8803_REGISTER_EXTENSION,
					 RV8803_EXTENSION_MASK_COUNTER,
					 RV8803_COUNTER_ENABLE_COUNTER);
	if (err < 0) {
		return err;
	}

	return 0;
}

static int rv8803_cnt_stop(const struct device *dev)
{
	const struct rv8803_cnt_config *cnt_config = dev->config;
	int err;

	err = rv8803_bus_reg_update_byte(cnt_config->mfd_dev, RV8803_REGISTER_EXTENSION,
					 RV8803_EXTENSION_MASK_COUNTER,
					 RV8803_COUNTER_DISABLE_COUNTER);
	if (err < 0) {
		return err;
	}

	return 0;
}

static int rv8803_cnt_set_top_value(const struct device *dev, const struct counter_top_cfg *cfg)
{
	const struct rv8803_cnt_config *cnt_config = dev->config;
	int err;

	if ((cfg->ticks <= 0) || (cfg->ticks >= RV8803_COUNTER_MAX_TOP_VALUE)) {
		LOG_ERR("WRONG ticks [%d][%d]", cfg->ticks, RV8803_COUNTER_MAX_TOP_VALUE);
		return -EINVAL;
	}

	/* TE, TIE and TF to 0 : stop interrupt */
	err = rv8803_bus_reg_update_byte(cnt_config->mfd_dev, RV8803_REGISTER_EXTENSION,
					 RV8803_EXTENSION_MASK_COUNTER,
					 RV8803_COUNTER_DISABLE_COUNTER);
	if (err < 0) {
		return err;
	}
	err = rv8803_bus_reg_update_byte(cnt_config->mfd_dev, RV8803_REGISTER_CONTROL,
					 RV8803_CONTROL_MASK_COUNTER,
					 RV8803_COUNTER_DISABLE_COUNTER);
	if (err < 0) {
		return err;
	}
	err = rv8803_bus_reg_update_byte(cnt_config->mfd_dev, RV8803_REGISTER_FLAG,
					 RV8803_FLAG_MASK_COUNTER, RV8803_COUNTER_DISABLE_COUNTER);
	if (err < 0) {
		return err;
	}

	/* Choose TD clock frequency */
	uint8_t value;
	switch (cnt_config->info.freq) {
	case 4096:
		value = RV8803_COUNTER_FREQUENCY_4096_HZ;
		break;

	case 64:
		value = RV8803_COUNTER_FREQUENCY_64_HZ;
		break;

	case 1:
		value = RV8803_COUNTER_FREQUENCY_1_HZ;
		break;

	default:
		LOG_ERR("WRONG Freq");
		return -EINVAL;
	}
	err = rv8803_bus_reg_update_byte(cnt_config->mfd_dev, RV8803_REGISTER_EXTENSION,
					 RV8803_COUNTER_FREQUENCY_MASK_COUNTER, value);
	if (err < 0) {
		return err;
	}

	/* Choose TC0/TC1 counter period */
	value = cfg->ticks & 0xFF;
	err = rv8803_bus_reg_write_byte(cnt_config->mfd_dev,
					RV8803_COUNTER_REGISTER_TIMER_COUNTER_0, value);
	if (err < 0) {
		return err;
	}
	value = (cfg->ticks >> 8) & 0x0F;
	err = rv8803_bus_reg_update_byte(cnt_config->mfd_dev,
					 RV8803_COUNTER_REGISTER_TIMER_COUNTER_1, 0x0F, value);
	if (err < 0) {
		return err;
	}

#if RV8803_HAS_IRQ
	struct rv8803_cnt_data *cnt_data = dev->data;

	if (cnt_data->cnt_irq != NULL) {
		/* TIE to 1 : enable interrupt */
		err = rv8803_bus_reg_update_byte(cnt_config->mfd_dev, RV8803_REGISTER_CONTROL,
						 RV8803_CONTROL_MASK_COUNTER,
						 RV8803_COUNTER_ENABLE_COUNTER);
		if (err < 0) {
			return err;
		}

		if (cnt_data->cnt_counter != NULL) {
			/* Register callback */
			cnt_data->cnt_counter->counter_cb = cfg->callback;
			cnt_data->cnt_counter->counter_cb_data = cfg->user_data;
		}
	}
#endif

	return 0;
}

static uint32_t rv8803_cnt_get_top_value(const struct device *dev)
{
	const struct rv8803_cnt_config *cnt_config = dev->config;
	uint8_t regs[2];
	int err;

	err = rv8803_bus_burst_read(cnt_config->mfd_dev, RV8803_COUNTER_REGISTER_TIMER_COUNTER_0,
				    regs, sizeof(regs));
	if (err < 0) {
		return err;
	}

	return (((regs[1] & 0x0F) << 8) | ((regs[0] & 0xFF) << 0));
}

static uint32_t rv8803_cnt_get_pending_int(const struct device *dev)
{
	const struct rv8803_cnt_config *cnt_config = dev->config;
	uint8_t reg;
	int err;

	err = rv8803_bus_reg_read_byte(cnt_config->mfd_dev, RV8803_REGISTER_FLAG, &reg);
	if (err < 0) {
		return err;
	}

	if (reg & RV8803_COUNTER_ENABLE_COUNTER) {
		err = rv8803_bus_reg_update_byte(cnt_config->mfd_dev, RV8803_REGISTER_FLAG,
						 RV8803_FLAG_MASK_COUNTER,
						 RV8803_COUNTER_DISABLE_COUNTER);
		if (err < 0) {
			LOG_ERR("GPIO worker I2C update ALARM FLAG error");
		}

		return 1;
	}

	return 0;
}

#if RV8803_HAS_IRQ
static void rv8803_cnt_worker(struct k_work *p_work)
{
	struct rv8803_cnt_irq *data = CONTAINER_OF(p_work, struct rv8803_cnt_irq, work);
	const struct rv8803_cnt_config *cnt_config = data->dev->config;
	const struct rv8803_cnt_data *cnt_data = data->dev->data;

	uint8_t reg;
	int err;

	LOG_DBG("Process Counter worker from interrupt");

	err = rv8803_bus_reg_read_byte(cnt_config->mfd_dev, RV8803_REGISTER_FLAG, &reg);
	if (err < 0) {
		LOG_ERR("COUNTER worker I2C read FLAGS error");
	}

	if (reg & RV8803_COUNTER_ENABLE_COUNTER) {
		LOG_DBG("FLAG Counter");
		if (cnt_data->cnt_counter != NULL && cnt_data->cnt_counter->counter_cb != NULL) {
			LOG_DBG("Calling Counter callback");
			cnt_data->cnt_counter->counter_cb(data->dev,
							  cnt_data->cnt_counter->counter_cb_data);

			err = rv8803_bus_reg_update_byte(cnt_config->mfd_dev, RV8803_REGISTER_FLAG,
							 RV8803_FLAG_MASK_COUNTER,
							 RV8803_COUNTER_DISABLE_COUNTER);
			if (err < 0) {
				LOG_ERR("GPIO worker I2C update COUNTER FLAG error");
			}
		}
	}
}
#endif /* RV8803_HAS_IRQ */

/* RV8803 CNT init */
static int rv8803_cnt_init(const struct device *dev)
{
	const struct rv8803_cnt_config *cnt_config = dev->config;

	if (!device_is_ready(cnt_config->mfd_dev)) {
		return -ENODEV;
	}
	LOG_INF("RV8803 CNT: FREQ[%d]", cnt_config->info.freq);
	LOG_INF("RV8803 CNT INIT");

#if RV8803_HAS_IRQ
	struct rv8803_cnt_data *cnt_data = dev->data;
	/**
	 * This block prevents the compiler from warning about unused functions
	 * when the corresponding features are not enabled.
	 */
	if (cnt_data->cnt_irq == NULL) {
		(void)rv8803_cnt_worker;
	} else if (cnt_data->cnt_irq->work.handler == NULL) {
		(void)rv8803_cnt_worker;
	}

	if ((cnt_data->cnt_irq != NULL) && (cnt_data->cnt_irq->append_listener_fn != NULL)) {
		cnt_data->cnt_irq->append_listener_fn(cnt_config->mfd_dev,
						      &cnt_data->cnt_irq->work);
	}
#endif /* RV8803_HAS_IRQ */

	return 0;
}

/* RV8803 CNT driver API */
static const struct counter_driver_api rv8803_cnt_driver_api = {
	.start = rv8803_cnt_start,
	.stop = rv8803_cnt_stop,
	.set_top_value = rv8803_cnt_set_top_value,
	.get_top_value = rv8803_cnt_get_top_value,
	.get_pending_int = rv8803_cnt_get_pending_int,
};

#define RV8803_COUNTER_IRQ_INIT(n)                                                                 \
	IF_ENABLED(RV8803_COUNTER_IRQ_HAS_PROP(n),                                                 \
		   (static struct rv8803_cnt_irq rv8803_cnt_irq_##n = {                            \
			    .dev = DEVICE_DT_GET(DT_DRV_INST(n)),                                  \
			    .work.handler = rv8803_cnt_worker,                                     \
			    COND_CODE_1(DT_PROP(DT_DRV_INST(n), use_irq),                          \
					(.append_listener_fn = rv8803_append_irq_listener, ),      \
					(.append_listener_fn = NULL, ))};))

#define RV8803_COUNTER_CALLBACK_INIT(n)                                                            \
	IF_ENABLED(RV8803_COUNTER_IRQ_HAS_PROP(n),                                                 \
		   (static struct rv8803_cnt_counter rv8803_cnt_counter_##n = {                    \
			    .counter_cb = NULL,                                                    \
			    .counter_cb_data = NULL,                                               \
		    };))

#define RV8803_COUNTER_CONFIG_INIT(n)                                                              \
	static const struct rv8803_cnt_config rv8803_cnt_config_##n = {                            \
		.info =                                                                            \
			{                                                                          \
				.max_top_value = RV8803_COUNTER_MAX_TOP_VALUE,                     \
				.freq = DT_INST_PROP(n, frequency),                                \
				.channels = RV8803_COUNTER_CHANNELS,                               \
			},                                                                         \
		.mfd_dev = DEVICE_DT_GET(DT_PARENT(DT_DRV_INST(n))),                               \
	};

#define RV8803_COUNTER_DATA_INIT(n)                                                                \
	static struct rv8803_cnt_data rv8803_cnt_data_##n = {COND_CODE_1(                          \
		RV8803_COUNTER_IRQ_HAS_PROP(n),                                                    \
		(.cnt_irq = &rv8803_cnt_irq_##n, .cnt_counter = &rv8803_cnt_counter_##n, ),        \
		(.cnt_irq = NULL, .cnt_counter = NULL, ))};

#define RV8803_COUNTER_STRUCT_CHECK(n)                                                             \
	BUILD_ASSERT(offsetof(struct rv8803_cnt_config, info) == 0,                                \
		     "ERROR microcrystal,rv8803-counter counter_config_info is not first. "        \
		     "counter_config_info must be first in the config structure.");

#define RV8803_COUNTER_INIT(n)                                                                     \
	RV8803_COUNTER_IRQ_INIT(n)                                                                 \
	RV8803_COUNTER_CALLBACK_INIT(n)                                                            \
	RV8803_COUNTER_CONFIG_INIT(n)                                                              \
	RV8803_COUNTER_DATA_INIT(n)                                                                \
	RV8803_COUNTER_STRUCT_CHECK(n)                                                             \
	DEVICE_DT_INST_DEFINE(n, rv8803_cnt_init, NULL, &rv8803_cnt_data_##n,                      \
			      &rv8803_cnt_config_##n, POST_KERNEL, CONFIG_COUNTER_INIT_PRIORITY,   \
			      &rv8803_cnt_driver_api);

DT_INST_FOREACH_STATUS_OKAY(RV8803_COUNTER_INIT)
#undef DT_DRV_COMPAT
