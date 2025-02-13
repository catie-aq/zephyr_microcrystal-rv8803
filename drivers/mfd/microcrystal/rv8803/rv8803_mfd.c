/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include "rv8803_api.h"
#define DT_DRV_COMPAT microcrystal_rv8803
#include "rv8803_mfd.h"

LOG_MODULE_REGISTER(RV8803, CONFIG_MFD_LOG_LEVEL);

int rv8803_bus_reg_read_byte(const struct device *dev, uint8_t reg_addr, uint8_t *value)
{
	const struct rv8803_config *config = dev->config;

	int err = i2c_reg_read_byte_dt(&config->i2c_bus, reg_addr, value);
	if (err < 0) {
		LOG_ERR("[ERROR]: Register read byte [0x%02X]", reg_addr);
	}

	return err;
}

int rv8803_bus_reg_write_byte(const struct device *dev, uint8_t reg_addr, uint8_t value)
{
	const struct rv8803_config *config = dev->config;

	int err = i2c_reg_write_byte_dt(&config->i2c_bus, reg_addr, value);
	if (err < 0) {
		LOG_ERR("[ERROR]: Register write byte [0x%02X]", reg_addr);
	}

	return err;
}

int rv8803_bus_reg_update_byte(const struct device *dev, uint8_t reg_addr, uint8_t mask,
			       uint8_t value)
{
	const struct rv8803_config *config = dev->config;

	int err = i2c_reg_update_byte_dt(&config->i2c_bus, reg_addr, mask, value);
	if (err < 0) {
		LOG_ERR("[ERROR]: Register update byte [0x%02X]", reg_addr);
	}

	return err;
}

int rv8803_bus_burst_read(const struct device *dev, uint8_t start_addr, uint8_t *buf,
			  uint32_t num_bytes)
{
	const struct rv8803_config *config = dev->config;

	int err = i2c_burst_read_dt(&config->i2c_bus, start_addr, buf, num_bytes);
	if (err < 0) {
		LOG_ERR("[ERROR]: Register burst read [0x%02X]-[0x%02X]", start_addr,
			start_addr + num_bytes);
	}

	return err;
}

int rv8803_bus_burst_write(const struct device *dev, uint8_t start_addr, const uint8_t *buf,
			   uint32_t num_bytes)
{
	const struct rv8803_config *config = dev->config;

	int err = i2c_burst_write_dt(&config->i2c_bus, start_addr, buf, num_bytes);
	if (err < 0) {
		LOG_ERR("[ERROR]: Register burst write [0x%02X]-[0x%02X]", start_addr,
			start_addr + num_bytes);
	}

	return err;
}

#if RV8803_HAS_IRQ
bool rv8803_irq_gpio_is_available(const struct device *dev)
{
	const struct rv8803_config *config = dev->config;
	return gpio_is_ready_dt(&config->irq_gpio);
}

int rv8803_append_irq_listener(const struct device *dev, struct k_work *worker)
{
	struct rv8803_data *data = dev->data;

	if (data->irq->workers_index >= data->irq->max_workers) {
		LOG_ERR("Listner array is full! [%d]", data->irq->max_workers);
		return -ENOSR;
	}

	LOG_INF("INDEX: [%d][%d]", data->irq->workers_index, data->irq->max_workers);
	data->irq->workers[data->irq->workers_index] = worker;
	data->irq->workers_index++;

	return 0;
}

static void rv8803_gpio_callback_handler(const struct device *p_port, struct gpio_callback *p_cb,
					 gpio_port_pins_t pins)
{
	ARG_UNUSED(p_port);
	ARG_UNUSED(pins);

	struct rv8803_irq *data = CONTAINER_OF(p_cb, struct rv8803_irq, gpio_cb);

	for (int i = 0; i < data->workers_index; i++) {
		if (data->workers[i] != NULL) {
			k_work_submit(data->workers[i]);
		}
	}
}
#else
bool rv8803_irq_gpio_is_available(const struct device *dev)
{
	return false;
}

int rv8803_append_irq_listener(const struct device *dev, struct k_work *worker)
{
	return -ENOTSUP;
}
#endif /* RV8803_HAS_IRQ */

#if CONFIG_MFD_RV8803_DETECT_BATTERY_STATE
static int rv8803_detect_battery_state(const struct device *dev)
{
	const struct rv8803_config *config = dev->config;
	struct rv8803_data *data = dev->data;
	uint8_t value;
	int err;

	err = i2c_reg_read_byte_dt(&config->i2c_bus, RV8803_REGISTER_FLAG, &value);
	if (err < 0) {
		LOG_ERR("Failed to read FLAGS register!!");
		return err;
	}
	LOG_DBG("FLAG REGISTER: [0x%02X]",
		value & (RV8803_FLAG_MASK_LOW_VOLTAGE_1 | RV8803_FLAG_MASK_LOW_VOLTAGE_2));
	data->bat.power_on_reset = (value & RV8803_FLAG_MASK_LOW_VOLTAGE_2) >> 1;
	data->bat.low_battery = value & RV8803_FLAG_MASK_LOW_VOLTAGE_1;

	if (data->bat.power_on_reset) {
		value &= ~RV8803_FLAG_MASK_LOW_VOLTAGE_2;
		err = i2c_reg_write_byte_dt(&config->i2c_bus, RV8803_REGISTER_FLAG, value);
		if (err < 0) {
			LOG_ERR("Failed to write FLAGS register!!");
			return err;
		}
	} else if (data->bat.low_battery) {
		value &= ~RV8803_FLAG_MASK_LOW_VOLTAGE_1;
		err = i2c_reg_write_byte_dt(&config->i2c_bus, RV8803_REGISTER_FLAG, value);
		if (err < 0) {
			LOG_ERR("Failed to write FLAGS register!!");
			return err;
		}
	}

	return 0;
}
#endif

/* RV8803 base init */
static int rv8803_init(const struct device *dev)
{
	const struct rv8803_config *config = dev->config;

	if (!i2c_is_ready_dt(&config->i2c_bus)) {
		LOG_ERR("I2C bus not ready!!");
		return -ENODEV;
	}

	k_sleep(K_MSEC(RV8803_STARTUP_TIMING_MS));

	/* Reset control register -> disable interrupts */
	int err =
		rv8803_bus_reg_update_byte(dev, RV8803_REGISTER_CONTROL,
					   RV8803_CONTROL_MASK_INTERRUPT, RV8803_DISABLE_INTERRUPT);
	if (err < 0) {
		LOG_ERR("Update CONTROL: [%d]", err);
		return err;
	}

#if CONFIG_MFD_RV8803_DETECT_BATTERY_STATE
	err = rv8803_detect_battery_state(dev);
	if (err < 0) {
		LOG_ERR("Failed to detect battery state!!");
		return err;
	}
#endif /* CONFIG_RV8803_DETECT_BATTERY_STATE */

#if RV8803_HAS_IRQ
	struct rv8803_data *data = dev->data;

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

	gpio_init_callback(&data->irq->gpio_cb, rv8803_gpio_callback_handler,
			   BIT(config->irq_gpio.pin));

	err = gpio_add_callback_dt(&config->irq_gpio, &data->irq->gpio_cb);
	if (err < 0) {
		LOG_ERR("Failed to add GPIO callback!!");
		return err;
	}
	LOG_INF("IRQ GPIO configured");
#endif /* RV8803_HAS_IRQ */

	LOG_INF("RV8803 INIT");

	return 0;
}

// clang-format off
#define RV8803_MFD_IRQ_INIT(n) \
	IF_ENABLED(RV8803_DT_CHILD_HAS_IRQ(n), ( \
		static struct k_work *rv8803_listener_##n[RV8803_DT_NUM_CHILD_IRQ(n)]; \
		static struct rv8803_irq rv8803_irq_##n = { \
			.workers = rv8803_listener_##n, \
			.workers_index = 0, \
			.max_workers = ARRAY_SIZE(rv8803_listener_##n), \
		}; \
	))

#define RV8803_MFD_CONFIG_INIT(n) \
	static const struct rv8803_config rv8803_config_##n = { \
		.i2c_bus = I2C_DT_SPEC_INST_GET(n), \
		IF_ENABLED(RV8803_DT_CHILD_HAS_IRQ(n), ( \
			.irq_gpio = GPIO_DT_SPEC_INST_GET_OR(n, irq_gpios, 0), \
		)) \
	};

#define RV8803_MFD_DATA_INIT(n) \
	static struct rv8803_data rv8803_data_##n = { \
		IF_ENABLED(CONFIG_MFD_RV8803_DETECT_BATTERY_STATE, \
			(.bat  = { \
			.power_on_reset = false, \
			.low_battery = false, \
		},) \
		) \
		COND_CODE_1(RV8803_HAS_IRQ, \
			(.irq = &rv8803_irq_##n,), \
			(.irq = NULL,) \
		) \
	};

#define RV8803_MFD_STRUCT_CHECK(n) \
	IF_ENABLED(CONFIG_MFD_RV8803_DETECT_BATTERY_STATE, ( \
		BUILD_ASSERT(offsetof(struct rv8803_data, bat) == 0, \
			"ERROR microcrystal,rv8803-mfd rv8803_battery is not first. rv8803_battery must be first in the data structure." \
		); \
	))

#define RV8803_MFD_INIT(n) \
	RV8803_MFD_IRQ_INIT(n) \
	RV8803_MFD_CONFIG_INIT(n) \
	RV8803_MFD_DATA_INIT(n) \
	RV8803_MFD_STRUCT_CHECK(n) \
	DEVICE_DT_INST_DEFINE(n, rv8803_init, NULL, &rv8803_data_##n, &rv8803_config_##n, \
			      POST_KERNEL, CONFIG_I2C_INIT_PRIORITY, NULL);
// clang-format on

DT_INST_FOREACH_STATUS_OKAY(RV8803_MFD_INIT)
#undef DT_DRV_COMPAT
