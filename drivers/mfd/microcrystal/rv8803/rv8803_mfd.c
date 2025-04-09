/*
 * Copyright (c) 2024, CATIE
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include "rv8803.h"
#include "rv8803_common.h"
#define DT_DRV_COMPAT microcrystal_rv8803
#include "rv8803_mfd.h"

LOG_MODULE_REGISTER(RV8803, CONFIG_MFD_LOG_LEVEL);

#if CONFIG_MFD_RV8803_WORKQUEUE
K_THREAD_STACK_DEFINE(rv8803_work_queue_stack, CONFIG_MFD_RV8803_WORKQUEUE_SIZE);
static struct k_work_q rv8803_work_queue;

/**
 * @brief Initializes the RV8803 work queue
 *
 * Initializes the work queue that handles RV8803 interrupts.
 *
 * @retval 0 on success
 */
static int rv8803_workqueue_init(void)
{
	k_work_queue_init(&rv8803_work_queue);
	k_work_queue_start(&rv8803_work_queue, rv8803_work_queue_stack,
			   K_THREAD_STACK_SIZEOF(rv8803_work_queue_stack),
			   CONFIG_MFD_RV8803_WORKQUEUE_PRIORITY, NULL);

	return 0;
}

/* The work-queue is shared across all instances, hence we initialize it separatedly */
SYS_INIT(rv8803_workqueue_init, POST_KERNEL, CONFIG_I2C_INIT_PRIORITY);
#endif

int rv8803_bus_reg_read_byte(const struct device *dev, uint8_t reg_addr, uint8_t *value)
{
	const struct rv8803_config *config = dev->config;

	int err = i2c_reg_read_byte_dt(&config->i2c_bus, reg_addr, value);
	if (err < 0) {
		LOG_ERR("[ERROR]: Register read byte [0x%02X]", reg_addr);
	}

	return err;
}

/**
 * @brief Write a byte to the RV8803 register
 *
 * @param dev RV8803 device structure
 * @param reg_addr Register address
 * @param value Value to write
 *
 * @retval 0 on success
 * @retval negative errno code on failure
 */
int rv8803_bus_reg_write_byte(const struct device *dev, uint8_t reg_addr, uint8_t value)
{
	const struct rv8803_config *config = dev->config;

	int err = i2c_reg_write_byte_dt(&config->i2c_bus, reg_addr, value);
	if (err < 0) {
		LOG_ERR("[ERROR]: Register write byte [0x%02X]", reg_addr);
	}

	return err;
}

/**
 * @brief Update a byte in the RV8803 register
 *
 * @param dev RV8803 device structure
 * @param reg_addr Register address
 * @param mask Mask of bits to update
 * @param value Value to write
 *
 * @retval 0 on success
 * @retval negative errno code on failure
 */
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

/**
 * @brief Perform a burst read from the RV8803 registers
 *
 * @param dev RV8803 device structure
 * @param start_addr Starting register address for the burst read
 * @param buf Buffer to store the read data
 * @param num_bytes Number of bytes to read
 *
 * @retval 0 on success
 * @retval negative errno code on failure
 */
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

/**
 * @brief Perform a burst write from the RV8803 registers
 *
 * @param dev RV8803 device structure
 * @param start_addr Starting register address for the burst write
 * @param buf Buffer to store the written data
 * @param num_bytes Number of bytes to write
 *
 * @retval 0 on success
 * @retval negative errno code on failure
 */
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
/**
 * @brief Check if the IRQ GPIO is available.
 *
 * This function checks if the IRQ GPIO is available and ready for use.
 *
 * @param dev Pointer to the RV8803 device structure.
 *
 * @return true if the IRQ GPIO is available, false otherwise.
 */
bool rv8803_irq_gpio_is_available(const struct device *dev)
{
	const struct rv8803_config *config = dev->config;
	return gpio_is_ready_dt(&config->irq_gpio);
}

/**
 * @brief Append an IRQ listener to the RV8803 device.
 *
 * Adds a new work item to the IRQ listener array for the RV8803 device.
 * Ensures that the listener array is not full before appending the new listener.
 * If the array is full, an error code is returned.
 *
 * @param dev Pointer to the RV8803 device structure.
 * @param worker Pointer to the work item to be appended as an IRQ listener.
 *
 * @retval 0 on success.
 * @retval -ENOSR if the listener array is full.
 */
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

/**
 * @brief Handle the GPIO interrupt callback.
 *
 * This function is called when a GPIO interrupt occurs. It iterates over the
 * list of registered workers and submits them to be executed.
 *
 * @param p_port Pointer to the GPIO device that triggered the interrupt.
 * @param p_cb Pointer to the GPIO callback structure.
 * @param pins Bitmask of pins that triggered the interrupt.
 */
static void rv8803_gpio_callback_handler(const struct device *p_port, struct gpio_callback *p_cb,
					 gpio_port_pins_t pins)
{
	ARG_UNUSED(p_port);
	ARG_UNUSED(pins);

	struct rv8803_irq *data = CONTAINER_OF(p_cb, struct rv8803_irq, gpio_cb);

	for (int i = 0; i < data->workers_index; i++) {
		if (data->workers[i] != NULL) {
#if CONFIG_MFD_RV8803_WORKQUEUE
			k_work_submit_to_queue(&rv8803_work_queue, data->workers[i]);
#else
			k_work_submit(data->workers[i]);
#endif
		}
	}
}
#else
/**
 * @brief Check if the GPIO interrupt is available.
 *
 * This function checks if the GPIO interrupt is available for the given device.
 *
 * @param dev Pointer to the device structure.
 *
 * @returns true if the GPIO interrupt is available, false otherwise.
 */
bool rv8803_irq_gpio_is_available(const struct device *dev)
{
	return false;
}

/**
 * @brief Appends a worker to the list of IRQ listeners.
 *
 * This function appends a worker to the list of IRQ listeners for the given
 * device. The worker will be executed when the GPIO interrupt is triggered.
 *
 * @param dev Pointer to the device structure.
 * @param worker Pointer to the worker to append.
 *
 * @returns -ENOTSUP if the GPIO interrupt is not available.
 */
int rv8803_append_irq_listener(const struct device *dev, struct k_work *worker)
{
	return -ENOTSUP;
}
#endif /* RV8803_HAS_IRQ */

#if CONFIG_MFD_RV8803_DETECT_BATTERY_STATE
/**
 * @brief Detects the battery state of the RV8803 device.
 *
 * This function reads the FLAGS register of the RV8803 device to determine
 * the battery state. It checks for power-on reset and low battery conditions
 * and updates the device's battery status accordingly. If a power-on reset or
 * low battery condition is detected, the corresponding flag in the FLAGS
 * register is cleared.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @return 0 on success, negative error code on failure.
 */
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

/**
 * @brief Initialize the RV8803 device.
 *
 * This function initializes the RV8803 device by:
 *  1. Checking if the I2C bus is ready.
 *  2. Sleeping for the startup timing.
 *  3. Disabling interrupts by setting the control register.
 *  4. Configuring the IRQ GPIO pin if interrupts are enabled.
 *  5. Detecting the battery state if the feature is enabled.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 on success
 * @retval -ENODEV if the I2C bus is not ready
 * @retval -EINVAL if the configuration is invalid
 * @retval -EIO if an I/O error occurs
 * @retval negative errno code on failure
 */
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

/**
 * @brief Macro to initialize the rv8803_battery struct
 *
 * This macro initializes the rv8803_battery struct which contains the
 * power-on reset and low battery status of the RV8803 device. It is
 * used when the CONFIG_MFD_RV8803_DETECT_BATTERY_STATE option is
 * enabled.
 *
 * @param n The instance number
 */
#define RV8803_MFD_IRQ_INIT(n)                                                                     \
	IF_ENABLED(RV8803_DT_CHILD_HAS_IRQ(n),                                                     \
		   (static struct k_work * rv8803_listener_##n[RV8803_DT_NUM_CHILD_IRQ(n)];        \
		    static struct rv8803_irq rv8803_irq_##n = {                                    \
			    .workers = rv8803_listener_##n,                                        \
			    .workers_index = 0,                                                    \
			    .max_workers = ARRAY_SIZE(rv8803_listener_##n),                        \
		    };))

/**
 * @brief Macro to initialize the rv8803_config struct
 *
 * This macro initializes the rv8803_config struct which contains the
 * I2C bus and the IRQ GPIO pin of the RV8803 device. It is used when
 * the RV8803_HAS_IRQ option is enabled.
 *
 * @param n The instance number
 */
#define RV8803_MFD_CONFIG_INIT(n)                                                                  \
	static const struct rv8803_config rv8803_config_##n = {                                    \
		.i2c_bus = I2C_DT_SPEC_INST_GET(n),                                                \
		IF_ENABLED(RV8803_DT_CHILD_HAS_IRQ(n),                                             \
			   (.irq_gpio = GPIO_DT_SPEC_INST_GET_OR(n, irq_gpios, 0), ))};

/**
 * @brief Macro to initialize the rv8803_data struct
 *
 * This macro initializes the rv8803_data struct with pointers to the
 * rv8803_battery and rv8803_irq structs based on the configuration
 * options. The pointers are set to NULL if the corresponding features
 * are not enabled in the configuration.
 *
 * @param n The instance number
 */
#define RV8803_MFD_DATA_INIT(n)                                                                    \
	static struct rv8803_data rv8803_data_##n = {IF_ENABLED(                                   \
		CONFIG_MFD_RV8803_DETECT_BATTERY_STATE,                                            \
		(.bat = {                                                                          \
			 .power_on_reset = false,                                                  \
			 .low_battery = false,                                                     \
		 }, )) COND_CODE_1(RV8803_HAS_IRQ, (.irq = &rv8803_irq_##n, ), (.irq = NULL, ))};

/**
 * @brief Macro to check that rv8803_battery is first in the rv8803_data struct
 *
 * This macro checks that the rv8803_battery struct is the first member of
 * the rv8803_data struct. This is required because we access the
 * rv8803_battery struct without a pointer in some places.
 *
 * @param n The instance number
 */
#define RV8803_MFD_STRUCT_CHECK(n)                                                                 \
	IF_ENABLED(CONFIG_MFD_RV8803_DETECT_BATTERY_STATE,                                         \
		   (BUILD_ASSERT(offsetof(struct rv8803_data, bat) == 0,                           \
				 "ERROR microcrystal,rv8803-mfd rv8803_battery is not first. "     \
				 "rv8803_battery must be first in the data structure.");))

/**
 * @brief Macro to generate the RV8803 MFD instance
 *
 * This macro generates the rv8803_config, rv8803_data, and rv8803_irq structs
 * for the given instance and registers the device with the kernel.
 *
 * @param n The instance number
 */
#define RV8803_MFD_INIT(n)                                                                         \
	RV8803_MFD_IRQ_INIT(n)                                                                     \
	RV8803_MFD_CONFIG_INIT(n)                                                                  \
	RV8803_MFD_DATA_INIT(n)                                                                    \
	RV8803_MFD_STRUCT_CHECK(n)                                                                 \
	DEVICE_DT_INST_DEFINE(n, rv8803_init, NULL, &rv8803_data_##n, &rv8803_config_##n,          \
			      POST_KERNEL, CONFIG_I2C_INIT_PRIORITY, NULL);

/**
 * @brief Generate the RV8803 MFD instances.
 *
 * This macro iterates over all compatible instances of the RV8803 MFD in the
 * Devicetree and initializes them. For each instance, it sets up the
 * configuration, data, and IRQ structures, and registers the device with the
 * kernel.
 *
 * @details The macro DT_INST_FOREACH_STATUS_OKAY is used to iterate over all
 * instances of the RV8803 MFD in the Devicetree. For each instance,
 * the RV8803_MFD_INIT macro is called to perform the necessary initialization
 * steps, including setting up the I2C bus, IRQ GPIO, and battery state
 * detection if enabled.
 */
DT_INST_FOREACH_STATUS_OKAY(RV8803_MFD_INIT)
#undef DT_DRV_COMPAT
