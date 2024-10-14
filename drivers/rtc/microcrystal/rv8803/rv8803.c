/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT microcrystal_rv8803

#include <zephyr/logging/log.h>

#include "rv8803.h"

LOG_MODULE_REGISTER(RV8803, CONFIG_RTC_LOG_LEVEL);

#if defined(RV8803_HAS_IRQ)
static void rv8803_gpio_callback_handler(const struct device *p_port, struct gpio_callback *p_cb,
					 gpio_port_pins_t pins)
{
	ARG_UNUSED(p_port);
	ARG_UNUSED(pins);

	struct rv8803_data *data = CONTAINER_OF(p_cb, struct rv8803_data, gpio_cb);

	if (data->rtc_work.handler != NULL) {
		k_work_submit(&data->rtc_work); /* Using work queue to exit isr context */
	}
	if (data->cnt_work.handler != NULL) {
		k_work_submit(&data->cnt_work); /* Using work queue to exit isr context */
	}
}
#endif /* RV8803_HAS_IRQ */

/* RV8803 base init */
static int rv8803_init(const struct device *dev)
{
	const struct rv8803_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->i2c_bus)) {
		LOG_ERR("I2C bus not ready!!");
		return -ENODEV;
	}

	k_sleep(K_MSEC(RV8803_STARTUP_TIMING_MS));

#if RV8803_HAS_IRQ || CONFIG_RV8803_DETECT_BATTERY_STATE
	struct rv8803_data *data = dev->data;
#endif

#if RV8803_HAS_IRQ
	if (!gpio_is_ready_dt(&config->irq_gpio)) {
		LOG_ERR("GPIO not ready!!");
		return -ENODEV;
	}

	LOG_INF("IRQ GPIO configure");
	err = gpio_pin_configure_dt(&config->irq_gpio, GPIO_INPUT);
	if (err < 0) {
		LOG_ERR("Failed to configure GPIO!!");
		return err;
	}

	err = gpio_pin_interrupt_configure_dt(&config->irq_gpio, GPIO_INT_EDGE_FALLING);
	if (err < 0) {
		LOG_ERR("Failed to configure interrupt!!");
		return err;
	}

	gpio_init_callback(&data->gpio_cb, rv8803_gpio_callback_handler, BIT(config->irq_gpio.pin));

	err = gpio_add_callback_dt(&config->irq_gpio, &data->gpio_cb);
	if (err < 0) {
		LOG_ERR("Failed to add GPIO callback!!");
		return err;
	}

	data->rtc_work.handler = NULL;
	data->cnt_work.handler = NULL;
#endif /* RV8803_HAS_IRQ */

#if CONFIG_RV8803_BATTERY_ENABLE
	uint8_t value;
	int err = i2c_reg_read_byte_dt(&config->i2c_bus, RV8803_REGISTER_FLAG, &value);
	if (err < 0) {
		LOG_ERR("Failed to read FLAGS register!!");
		return err;
	}
	LOG_DBG("FLAG REGISTER: [0x%02X]",
		value & (RV8803_FLAG_MASK_LOW_VOLTAGE_1 | RV8803_FLAG_MASK_LOW_VOLTAGE_2));
	data->power_on_reset = (value & RV8803_FLAG_MASK_LOW_VOLTAGE_2) >> 1;
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
		IF_ENABLED(RV8803_HAS_IRQ,                                                         \
			   (.irq_gpio = GPIO_DT_SPEC_INST_GET_OR(n, irq_gpios, {0}), ))};          \
	static struct rv8803_data rv8803_data_##n;                                                 \
	DEVICE_DT_INST_DEFINE(n, rv8803_init, NULL, &rv8803_data_##n, &rv8803_config_##n,          \
			      POST_KERNEL, CONFIG_RTC_INIT_PRIORITY, NULL);

/* Instanciate RV8803 */
DT_INST_FOREACH_STATUS_OKAY(RV8803_INIT)
#undef DT_DRV_COMPAT
